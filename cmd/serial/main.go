package main

import (
	"github.com/tarm/serial"
	"log"
	"time"
)

var expl = map[int]string{
	1: "стартовый байт",
	2: "байт комманды ответ",
	3: "старшие 8-м байт напряжения АКБ х 10",
	4: "младшие 8-м байт напряжения АКБ х 10",
	5: "ток АКБ х 10",
	6: "ток выходв х 10",
	7: "сеть 220В: 0х00 - нет сети, 0х01 - есть сеть, 0х02 - нет заземления",
	8: "асимметричный заряд: 0х00 - выключен, 0х01 - включен",
	9: "состояние заряда АКБ: 0х00 - АКБ отсутствует или авария цепи заряда, 0х01 - заряд включен, 0х02 - заряд ограничен из-за превышения выходной мощности блока",
	10: " модель блока питания UPS",
	11: "номер недели производства, если 0х00 - калибровка блока не проводилась",
	12: "год производства, если 0х00 - калибровка блока не проводилась",
	13: "старшие 8-мь бит серийного номера, 0х00 - калибровка блока не проводилась",
	14: "младшие 8-мь бит серийного номера, 0х00 - калибровка блока не проводилась",
	15: "версия програмного обеспечения, 0х00 - калибровка блока не проводилась",
	16: "CRC, контрольная сумма младших 8 бит всех предыдущий байт в посылке",
}


func main() {
	c := &serial.Config{
		Name: "/dev/cu.usbserial-00000000",
		Baud: 9600,
		Size: 8,
		StopBits: serial.Stop1,
		Parity: serial.ParityNone,
		ReadTimeout: 5*time.Second,
	}
	s, err := serial.OpenPort(c)
	if err != nil {
		log.Fatal(err)
	}

	_, err = s.Write([]byte{0x55, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x56}) // 0x56 - crc, sum of all bytes
	if err != nil {
		log.Fatal(err)
	}


	resp := make([]byte, 0, 30)
	for i:= 0; i < 29; {
		buf := make([]byte, 128)
		n, err := s.Read(buf)
		if err != nil {
			log.Fatal(err)
		}
		resp = append(resp, buf[:n]...)
		i += n
	}
	var sum byte
	for z := 13; z < 29; z++ {
		explanation, _ := expl[z - 12]
		log.Printf("%02d: %02X - %s\n", z - 12, resp[z], explanation)
		sum += resp[z]
	}
	log.Printf("Summory: %02X\n\n\n", sum)
}
