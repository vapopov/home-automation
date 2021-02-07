package main


import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"os/signal"
	"time"

	"github.com/brendoncarroll/go-can"
	"github.com/d2r2/go-i2c"
	"github.com/eclipse/paho.mqtt.golang"
)

var f mqtt.MessageHandler = func(client mqtt.Client, msg mqtt.Message) {
	fmt.Printf("TOPIC: %s\n", msg.Topic())
	fmt.Printf("MSG: %s\n", msg.Payload())
}

type Button struct {
	Action      string `json:"action"`
	Linkquality int    `json:"linkquality"`
}

type ButtonAqara struct {
	Action      string `json:"action"`
	Battery     int    `json:"battery"`
	Click       string `json:"click"`
	Linkquality int    `json:"linkquality"`
	Voltage     int    `json:"voltage"`
}

type Light struct {
	State string `json:"state"`
}

type RegChange struct {
	Mask uint16
	On bool
}

var (
	regState1 = uint16(0)
	regState2 = uint16(0)
	regState3 = uint16(0)
	regChan1  = make(chan RegChange)
	regChan2  = make(chan RegChange)
	regChan3  = make(chan RegChange)
)

func main() {
	mqtt.ERROR = log.New(os.Stdout, "[ERROR] ", 0)
	mqtt.CRITICAL = log.New(os.Stdout, "[CRIT] ", 0)
	mqtt.WARN = log.New(os.Stdout, "[WARN]  ", 0)

	opts := mqtt.NewClientOptions().AddBroker("tcp://localhost:1883")
	opts.SetKeepAlive(20 * time.Second)
	opts.SetDefaultPublishHandler(f)
	opts.SetPingTimeout(10 * time.Second)

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Create new connection to I2C bus.
	go spawnRegistry(ctx, 0x20, regChan1, &regState1)
	go spawnRegistry(ctx, 0x21, regChan2, &regState2)
	go spawnRegistry(ctx, 0x22, regChan3, &regState3)

	// Init can0 bus interface.
	bus, err := can.NewCANBus("can0")
	if err != nil {
		log.Fatal(err)
	}

	// Init mqtt client to the server.
	client := mqtt.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}

	fmt.Println("Sample Subscriber Started")
	
	if token := client.Subscribe("zigbee2mqtt/0x00124b000cc8e2ff", 0, func(c mqtt.Client, msg mqtt.Message) {
		b := new(Button)
		if err := json.Unmarshal(msg.Payload(), b); err != nil {
			fmt.Println(err)
			return
		}

		cf := can.CANFrame{}
		cf.SetAddr(0x200)
		cf.Len = uint32(8)

		switch b.Action {
		case "button_1_click":
			cf.Data = [8]byte{0, 0, 0, 0, 0, 0, 0, 1}
			if err := bus.Write(&cf); err != nil {
				fmt.Println(err)
			}
			regChan1 <- RegChange{
				Mask: 0xff,
				On:   true,
			}
			regChan2 <- RegChange{
				Mask: 0xff,
				On:   true,
			}

		case "button_2_click":
			cf.Data = [8]byte{0, 0, 0, 0, 0, 0, 0, 2}
			if err := bus.Write(&cf); err != nil {
				fmt.Println(err)
			}
			regChan1 <- RegChange{
				Mask: 0xff,
				On:   false,
			}
			regChan2  <- RegChange{
				Mask: 0xff,
				On:   false,
			}

		case "button_3_click":
			cf.Data = [8]byte{0, 0, 0, 0, 0, 0, 0, 3}
			if err := bus.Write(&cf); err != nil {
				fmt.Println(err)
			}
		case "button_4_click":
			cf.Data = [8]byte{0, 0, 0, 0, 0, 0, 0, 0}
			if err := bus.Write(&cf); err != nil {
				fmt.Println(err)
			}
		}
	}); token.Wait() && token.Error() != nil {
		fmt.Println(token.Error())
		os.Exit(1)
	}

	// Aquara button hardcoded behaviour.
	var leftOn, rightOn bool
	if token := client.Subscribe("zigbee2mqtt/0x00158d00033e1514", 0, func(client mqtt.Client, msg mqtt.Message) {
		b := new(ButtonAqara)
		if err := json.Unmarshal(msg.Payload(), b); err != nil {
			fmt.Println(err)
			return
		}
		switch b.Action {
		case "single_left":
			send := `{"state": "on"}`
			if leftOn {
				send = `{"state": "off"}`
			}
			leftOn = !leftOn
			for i := 1; i <= 8; i++ {
				if token := client.Publish(fmt.Sprintf("home/light%d", i), 0, false, []byte(send)); token.Wait() && token.Error() != nil {
					fmt.Println(token.Error())
				}
			}
		case "single_right":
			send := `{"state": "on"}`
			if rightOn {
				send = `{"state": "off"}`
			}
			rightOn = !rightOn
			for i := 9; i <= 16; i++ {
				if token := client.Publish(fmt.Sprintf("home/light%d", i), 0, false, []byte(send)); token.Wait() && token.Error() != nil {
					fmt.Println(token.Error())
				}
			}
		case "single_both":
			send := `{"state": "on"}`
			if leftOn {
				send = `{"state": "off"}`
			}
			leftOn = !leftOn
			for i := 1; i <= 8; i++ {
				if token := client.Publish(fmt.Sprintf("home/light%d", i), 0, false, []byte(send)); token.Wait() && token.Error() != nil {
					fmt.Println(token.Error())
				}
			}
			send = `{"state": "on"}`
			if rightOn {
				send = `{"state": "off"}`
			}
			rightOn = !rightOn
			for i := 9; i <= 16; i++ {
				if token := client.Publish(fmt.Sprintf("home/light%d", i), 0, false, []byte(send)); token.Wait() && token.Error() != nil {
					fmt.Println(token.Error())
				}
			}
		case "double_left":
		case "double_right":
		case "hold_left":
		case "hold_right":
		case "hold_both":
		case "double_both":
		}
	}); token.Wait() && token.Error() != nil {
		fmt.Println(token.Error())
		os.Exit(1)
	}

	subscribeLight(client, regChan1, "home/light1", 1)
	subscribeLight(client, regChan1, "home/light2", 1 << 1)
	subscribeLight(client, regChan1, "home/light3", 1 << 2)
	subscribeLight(client, regChan1, "home/light4", 1 << 3)
	subscribeLight(client, regChan1, "home/light5", 1 << 4)
	subscribeLight(client, regChan1, "home/light6", 1 << 5)
	subscribeLight(client, regChan1, "home/light7", 1 << 6)
	subscribeLight(client, regChan1, "home/light8", 1 << 7)

	subscribeLight(client, regChan2, "home/light9", 1)
	subscribeLight(client, regChan2, "home/light10", 1 << 1)
	subscribeLight(client, regChan2, "home/light11", 1 << 2)
	subscribeLight(client, regChan2, "home/light12", 1 << 3)
	subscribeLight(client, regChan2, "home/light13", 1 << 4)
	subscribeLight(client, regChan2, "home/light14", 1 << 5)
	subscribeLight(client, regChan2, "home/light15", 1 << 6)
	subscribeLight(client, regChan2, "home/light16", 1 << 7)

	subscribeLight(client, regChan3, "home/light17", 1)
	subscribeLight(client, regChan3, "home/light18", 1 << 1)
	subscribeLight(client, regChan3, "home/light19", 1 << 2)
	subscribeLight(client, regChan3, "home/light20", 1 << 3)
	subscribeLight(client, regChan3, "home/light21", 1 << 4)
	subscribeLight(client, regChan3, "home/light22", 1 << 5)
	subscribeLight(client, regChan3, "home/light23", 1 << 6)
	subscribeLight(client, regChan3, "home/light24", 1 << 7)

	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt)

	<-c
}

func spawnRegistry(ctx context.Context, addr uint8, regChan <-chan RegChange, regState *uint16) {
	dev, err := i2c.NewI2C(addr, 1)
	if err != nil {
		log.Fatal(err)
	}
	defer dev.Close()

	if err := dev.WriteRegU16LE(0, 0); err != nil {
		log.Fatal(err)
	}

	for {
		select {
		case <-ctx.Done():
			return
		case rg := <-regChan:
			if rg.On {
				*regState |= rg.Mask
			} else {
				*regState &= ^rg.Mask
			}
			if err := dev.WriteRegU16LE(0x14, *regState); err != nil {
				fmt.Println(err)
			}
		}
	}
}

func subscribeLight(client mqtt.Client, regChan chan<- RegChange, topic string, mask uint16) {
	if token := client.Subscribe(topic, 0, func(c mqtt.Client, msg mqtt.Message) {
		l := new(Light)
		if err := json.Unmarshal(msg.Payload(), l); err != nil {
			fmt.Println(err)
			return
		}

		switch l.State {
		case "on":
			regChan <- RegChange{
				On:   true,
				Mask: mask,
			}
		case "off":
			regChan <- RegChange{
				On:   false,
				Mask: mask,
			}
		}
	}); token.Wait() && token.Error() != nil {
		log.Fatal(token.Error())
	}
}