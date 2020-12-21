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

	mHandler := func(c mqtt.Client, msg mqtt.Message) {
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

		case "button_2_click":
			cf.Data = [8]byte{0, 0, 0, 0, 0, 0, 0, 2}
			if err := bus.Write(&cf); err != nil {
				fmt.Println(err)
			}
			regChan1 <- RegChange{
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
	}

	if token := client.Subscribe("zigbee2mqtt/0x00124b000cc8e2ff", 0, mHandler); token.Wait() && token.Error() != nil {
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