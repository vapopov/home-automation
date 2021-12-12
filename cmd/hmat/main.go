package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"os/signal"
	"time"

	can "github.com/brendoncarroll/go-can"
	"github.com/d2r2/go-i2c"
	"github.com/eclipse/paho.mqtt.golang"
)

var f mqtt.MessageHandler = func(client mqtt.Client, msg mqtt.Message) {
	fmt.Printf("TOPIC: %s\n", msg.Topic())
	fmt.Printf("MSG: %s\n", msg.Payload())
}

var (
	canListeners []func(frame can.CANFrame)
	topicStates  = make(map[string]bool)
)

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

func leftOn() bool {
	return regState1 > 0
}

func rightOn() bool {
	return regState2 > 0
}

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


	go func() {
		for {
			select {
			case <-ctx.Done():
				return
			default:
				cf := can.CANFrame{}
				if err := bus.Read(&cf); err == nil {
					for _, listener := range canListeners {
						listener(cf)
					}
					if cf.ID == 0x1ac {
						send := `{"state": "off"}`
						if cf.Data[1] > 0 {
							send = `{"state": "on"}`
						}
						for i := 1; i <= 16; i++ {
							if token := client.Publish(fmt.Sprintf("home/light%d", cf.Data[0]), 0, false, []byte(send)); token.Wait() && token.Error() != nil {
								fmt.Println(token.Error())
							}
						}
					}

					//if cf.ID == 0x101 && cf.Data[0] == 0x0 && cf.Data[1] == 0x1 {
					//	send := `{"state": "on"}`
					//	if leftOn() || rightOn() {
					//		send = `{"state": "off"}`
					//	}
					//	for i := 1; i <= 16; i++ {
					//		if token := client.Publish(fmt.Sprintf("home/light%d", i), 0, false, []byte(send)); token.Wait() && token.Error() != nil {
					//			fmt.Println(token.Error())
					//		}
					//	}
					//}
				}
			}
		}
	}()

	if token := client.Subscribe("zigbee2mqtt/0x00158d00033e1514", 0, func(client mqtt.Client, msg mqtt.Message) {
		b := new(ButtonAqara)
		if err := json.Unmarshal(msg.Payload(), b); err != nil {
			fmt.Println(err)
			return
		}
		switch b.Action {
		case "single_left":
			send := `{"state": "on"}`
			if leftOn() {
				send = `{"state": "off"}`
			}
			for i := 1; i <= 8; i++ {
				if token := client.Publish(fmt.Sprintf("home/light%d", i), 0, false, []byte(send)); token.Wait() && token.Error() != nil {
					fmt.Println(token.Error())
				}
			}
		case "single_right":
			send := `{"state": "on"}`
			if rightOn() {
				send = `{"state": "off"}`
			}
			for i := 9; i <= 16; i++ {
				if token := client.Publish(fmt.Sprintf("home/light%d", i), 0, false, []byte(send)); token.Wait() && token.Error() != nil {
					fmt.Println(token.Error())
				}
			}
		case "single_both":
			send := `{"state": "on"}`
			if leftOn() {
				send = `{"state": "off"}`
			}
			for i := 1; i <= 8; i++ {
				if token := client.Publish(fmt.Sprintf("home/light%d", i), 0, false, []byte(send)); token.Wait() && token.Error() != nil {
					fmt.Println(token.Error())
				}
			}
			send = `{"state": "on"}`
			if rightOn() {
				send = `{"state": "off"}`
			}
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

	canListeners = []func(frame can.CANFrame) {
		subCanFrameSwitcherOnOffAll(client, []string{
			"home/light1", "home/light2", "home/light3", "home/light4",
			"home/light5", "home/light6", "home/light7", "home/light8",
			"home/light9", "home/light12",
			"home/light13", "home/light14", "home/light15", "home/light16",
		}, 0x101, []byte{0x00, 0x01}),

		subCanFrameSwitcherOnOff(client, []string{"home/light8"}, 0x112, []byte{0x01, 0x01}),  // bathroom master bedroom light

		subCanFrameSwitcherOnOff(client, []string{"home/led_mb_toilet"}, 0x112, []byte{0x00, 0x01}),  // bathroom master bedroom light
		subCanFrameSwitcherOnOff(client, []string{"home/light11"}, 0x112, []byte{0x00, 0x02}), // bathroom master bedroom vents
	}

	subscribeLedStrips(client, bus, "home/led_mb_toilet", 0x112, []byte{0x04, 0x10, 0x90, 0x04, 0xf0}, []byte{0x03, 0x00})


	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt)

	<-c
}

func subCanFrameSwitcherOnOff(client mqtt.Client, topics []string, id uint32, dataSet ...[]byte) func(frame can.CANFrame) {
	return func(frame can.CANFrame) {
		if id != frame.ID {
			return
		}
		for _, data := range dataSet {
			matched := true
			for i, d := range data {
				if d != frame.Data[i] {
					matched = false
				}
			}
			if matched {
				for _, topic := range topics {
					send := `{"state": "on"}`
					if state, ok := topicStates[topic]; ok && state {
						send = `{"state": "off"}`
					}

					if token := client.Publish(topic, 0, false, []byte(send)); token.Wait() && token.Error() != nil {
						fmt.Println(token.Error())
					}
				}
			}
		}
	}
}

func subCanFrameSwitcherOnOffAll(client mqtt.Client, topics []string, id uint32, dataSet ...[]byte) func(frame can.CANFrame) {
	return func(frame can.CANFrame) {
		if id != frame.ID {
			return
		}
		for _, data := range dataSet {
			matched := true
			for i, d := range data {
				if d != frame.Data[i] {
					matched = false
				}
			}
			if matched {
				send := `{"state": "on"}`
				for _, topic := range topics {
					if state, ok := topicStates[topic]; ok && state {
						send = `{"state": "off"}`
						break
					}
				}
				for _, topic := range topics {
					if token := client.Publish(topic, 0, false, []byte(send)); token.Wait() && token.Error() != nil {
						fmt.Println(token.Error())
					}
				}
			}
		}
	}
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
		light := new(Light)
		if err := json.Unmarshal(msg.Payload(), light); err != nil {
			fmt.Println(err)
			return
		}

		switch light.State {
		case "on":
			regChan <- RegChange{
				On:   true,
				Mask: mask,
			}
			topicStates[topic] = true
		case "off":
			regChan <- RegChange{
				On:   false,
				Mask: mask,
			}
			topicStates[topic] = false
		}

	}); token.Wait() && token.Error() != nil {
		log.Fatal(token.Error())
	}
}

func subscribeLedStrips(client mqtt.Client, bus *can.CANBus, topic string, id uint32, dataOn []byte, dataOff []byte) {
	if token := client.Subscribe(topic, 0, func(c mqtt.Client, msg mqtt.Message) {
		light := new(Light)
		if err := json.Unmarshal(msg.Payload(), light); err != nil {
			fmt.Println(err)
			return
		}

		switch light.State {
		case "on":
			fr := &can.CANFrame{ID: id, Len: uint32(len(dataOn))}
			copy(fr.Data[:], dataOn)
			_ = bus.Write(fr)
			topicStates[topic] = true
		case "off":
			fr := &can.CANFrame{ID: id, Len: uint32(len(dataOff))}
			copy(fr.Data[:], dataOff)
			_ = bus.Write(fr)
			topicStates[topic] = false
		}
	}); token.Wait() && token.Error() != nil {
		log.Fatal(token.Error())
	}
}

// 1 - lodzhiya
// 2 - bra in kitchen ??
// 3 - ???
// 4 - cabinet under table
// 5 - ????
// 6 - warderobe cabinet
// 7 - master bedroom tunnel enterance
// 8 - toilet
// 9 - ????
// 10 - vityazka toilet guests
// 11 - vityazka toilet master bedroom
// 12 - server room, washing room
// 13 - toilet guests
// 14 - kitchen
// 15 - vhod
// 16 - kitchen near tracks
// 17 empty
// 18 empty
// 19 empty
// 20 empty
// 21 - ?????
// 22 - ?????
// 23 - ?????
// 24 - vityazka server room
