package main


import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"os/signal"
	"time"

	"github.com/brendoncarroll/go-can"
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

func main() {
	mqtt.ERROR = log.New(os.Stdout, "[ERROR] ", 0)
	mqtt.CRITICAL = log.New(os.Stdout, "[CRIT] ", 0)
	mqtt.WARN = log.New(os.Stdout, "[WARN]  ", 0)

	opts := mqtt.NewClientOptions().AddBroker("tcp://localhost:1883")
	opts.SetKeepAlive(20 * time.Second)
	opts.SetDefaultPublishHandler(f)
	opts.SetPingTimeout(10 * time.Second)

	bus, err := can.NewCANBus("can0")
	if err != nil {
		log.Fatal(err)
	}

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
				return
			}
		case "button_2_click":
			cf.Data = [8]byte{0, 0, 0, 0, 0, 0, 0, 2}
			if err := bus.Write(&cf); err != nil {
				fmt.Println(err)
				return
			}
		case "button_3_click":
			cf.Data = [8]byte{0, 0, 0, 0, 0, 0, 0, 3}
			if err := bus.Write(&cf); err != nil {
				fmt.Println(err)
				return
			}
		case "button_4_click":
			cf.Data = [8]byte{0, 0, 0, 0, 0, 0, 0, 0}
			if err := bus.Write(&cf); err != nil {
				fmt.Println(err)
				return
			}
		}
	}

	if token := client.Subscribe("zigbee2mqtt/0x00124b000cc8e2ff", 0, mHandler); token.Wait() && token.Error() != nil {
		fmt.Println(token.Error())
		os.Exit(1)
	}

	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt)

	<-c
}

