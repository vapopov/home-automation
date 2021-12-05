// CAN Send Example
//

#include <mcp_can.h>
#include <SPI.h>
#include <OneButton.h>

#define DEVICE_ADDR     0x101
#define CAN_FILTER_MASK 0x010F0000
#define CAN_FILTER      0x01010000

MCP_CAN CAN0(10);     // Set CS to pin 10
#define CAN0_INT 9    // Set INT to pin 9


#define BUTTON_ZERO 3
#define BUTTON_ONE 4
#define BUTTON_TWO 5

// To active CLK0 output from avr ************
// avrdude -c usbasp -p m328p -U lfuse:w:0xB7:m
// http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p&LOW=B7&HIGH=DA&EXTENDED=FD&LOCKBIT=FF  

byte data[8] = {0x00, 0x00};
//             -addr--inc-  -temp-----  -set temp-  -gap-
byte pong[7] = {0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];

/**
 * To verify connection to the chip:
 * avrdude -c usbasp -p m328p
 */

OneButton btn0 = OneButton(
  BUTTON_ZERO, // Input pin for the button
  true,        // Button is active LOW
  true         // Enable internal pull-up resistor
);

OneButton btn1 = OneButton(
  BUTTON_ONE,  // Input pin for the button
  true,        // Button is active LOW
  true         // Enable internal pull-up resistor
);

OneButton btn2 = OneButton(
 BUTTON_TWO,           // Input pin for the button
 true,        // Button is active LOW
 true         // Enable internal pull-up resistor
);

void setup()
{
  CAN0.begin(MCP_STD, CAN_125KBPS, MCP_16MHZ);

  CAN0.init_Mask(0,0,CAN_FILTER_MASK);           // Init first mask...
  CAN0.init_Filt(0,0,CAN_FILTER);                // Init second filter...
  CAN0.init_Filt(1,0,CAN_FILTER);                // Init second filter...

  CAN0.init_Mask(1,0,CAN_FILTER_MASK);
  CAN0.init_Filt(2,0,CAN_FILTER);
  CAN0.init_Filt(3,0,CAN_FILTER);
  CAN0.init_Filt(4,0,CAN_FILTER);
  CAN0.init_Filt(5,0,CAN_FILTER);

  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted

  pinMode(CAN0_INT, INPUT);

  // Single Click event attachment
  btn0.attachClick([]() {
    data[0] = 0;
    data[1] = 1;
    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
  });
  
  // Double Click event attachment with lambda
  btn0.attachDoubleClick([]() {
    data[0] = 0;
    data[1] = 2;
    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
  });

  btn0.attachLongPressStart([]() {
    data[0] = 0;
    data[1] = 3;
    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
  });
  
  btn0.attachLongPressStop([]() {
    data[0] = 0;
    data[1] = 4;
    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
  });

  // ------------------------------------------------
  // Single Click event attachment
  btn1.attachClick([]() {
    data[0] = 1;
    data[1] = 1;
    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
  });
  
  // Double Click event attachment with lambda
  btn1.attachDoubleClick([]() {
    data[0] = 1;
    data[1] = 2;
    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
  });

  btn1.attachLongPressStart([]() {
    data[0] = 1;
    data[1] = 3;
    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
  });
  
  btn1.attachLongPressStop([]() {
    data[0] = 1;
    data[1] = 4;
    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
  });

  // ------------------------------------------------
  // Single Click event attachment
 btn2.attachClick([]() {
   data[0] = 2;
   data[1] = 1;
   CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
 });

 // Double Click event attachment with lambda
 btn2.attachDoubleClick([]() {
   data[0] = 2;
   data[1] = 2;
   CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
 });

 btn2.attachLongPressStart([]() {
   data[0] = 2;
   data[1] = 3;
   CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
 });

 btn2.attachLongPressStop([]() {
   data[0] = 2;
   data[1] = 4;
   CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
 });
}


// ----------------------------------------- Main Loop --------------------------------------------
void loop()
{
  btn0.tick();
  btn1.tick();
  btn2.tick();

  if(!digitalRead(CAN0_INT)) {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
    // (rxId & DEVICE_ADDR) == DEVICE_ADDR)
    if((rxId & 0x40000000) == 0x40000000) {
      // Determine if message is a remote request frame.
       CAN0.sendMsgBuf(DEVICE_ADDR, 0, 7,  pong);
       pong[1]++;
    }
  }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
