// CAN Send Example
//

#include <mcp_can.h>
#include <SPI.h>
#include <OneButton.h>


MCP_CAN CAN0(10);     // Set CS to pin 10
#define CAN0_INT 9    // Set INT to pin 9

#define DEVICE_ADDR 0x104

#define PWM_LED 3
#define PIN_RELAY 8

byte data[2] = {0x00, 0x00};
byte pong[2] = {0xfe, 0x00};

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[2];

/**
 * To verify connection to the chip:
 * avrdude -c usbasp -p m328p
 * 
 * Four pins 10(SS), 11(MOSI), 12(MISO), and 13(SCK) are used for this purpose, 9 (INT), 10 (CS)
 * orange(11-MOSI), yellow (12-MISO), green (13-SCK), blue (RESET)
 */

/***********************************************************************************
If you send the following standard IDs below to an Arduino loaded with this sketch
you will find that 0x102 and 0x105 will not get in.
ID in Hex  -   Two Data Bytes!   -  Filter/Mask in HEX
   0x100   + 0000 0000 0000 0000 =   0x01000000
   0x101   + 0000 0000 0000 0000 =   0x01010000
   0x102   + 0000 0000 0000 0000 =   0x01020000  This example will NOT be receiving this ID
   0x103   + 0000 0000 0000 0000 =   0x01030000
   0x104   + 0000 0000 0000 0000 =   0x01040000
   0x105   + 0000 0000 0000 0000 =   0x01050000  This example will NOT be receiving this ID
   0x106   + 0000 0000 0000 0000 =   0x01060000
   0x107   + 0000 0000 0000 0000 =   0x01070000
   This mask will check the filters against ID bit 8 and ID bits 3-0.   
    MASK   + 0000 0000 0000 0000 =   0x010F0000
   
   If there is an explicit filter match to those bits, the message will be passed to the
   receive buffer and the interrupt pin will be set.
   This example will NOT be exclusive to ONLY the above frame IDs, for that a mask such
   as the below would be used: 
    MASK   + 0000 0000 0000 0000 = 0x07FF0000
    
   This mask will check the filters against all ID bits and the first data byte:
    MASK   + 1111 1111 0000 0000 = 0x07FFFF00
   If you use this mask and do not touch the filters below, you will find that your first
   data byte must be 0x00 for the message to enter the receive buffer.
   
   At the moment, to disable a filter or mask, copy the value of a used filter or mask.
   
   Data bytes are ONLY checked when the MCP2515 is in 'MCP_STDEXT' mode via the begin
   function, otherwise ('MCP_STD') only the ID is checked.
***********************************************************************************/


OneButton btn0 = OneButton(
  4,           // Input pin for the button
  true,        // Button is active LOW
  true         // Enable internal pull-up resistor
);

OneButton btn1 = OneButton(
  2,           // Input pin for the button
  true,        // Button is active LOW
  true         // Enable internal pull-up resistor
);

//OneButton btn2 = OneButton(
//  6,           // Input pin for the button
//  true,        // Button is active LOW
//  true         // Enable internal pull-up resistor
//);

void setup()
{
  Serial.begin(115200);

  // Initialize MCP2515 running at 8MHz with a baudrate of 125kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_STD, CAN_125KBPS, MCP_8MHZ) == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
  else Serial.println("Error Initializing MCP2515...");

  CAN0.init_Mask(0,0,0x010F0000);                // Init first mask...
  CAN0.init_Filt(0,0,0x01040000);                // Init second filter...
  CAN0.init_Filt(1,0,0x01040000);                // Init second filter...

  CAN0.init_Mask(1,0,0x010F0000);
  CAN0.init_Filt(2,0,0x01040000);
  CAN0.init_Filt(3,0,0x01040000);
  CAN0.init_Filt(4,0,0x01040000);
  CAN0.init_Filt(5,0,0x01040000);

  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted

  pinMode(CAN0_INT, INPUT);
  pinMode(PWM_LED, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);

  // ------------------------------ Set Timers -------------------------------------
  cli();
  TCCR1A = 0;             //Reset entire TCCR1A register
  TCCR1B = 0;             //Reset entire TCCR1B register
  TCCR1B |= B00000100;    //Set CS12 to 1 so we get Prescalar = 256
  TCNT1 = 0;              //Reset Timer 1 value to 0
  // TIMSK1 |= B00000010;    //Set OCIE1A to 1 so we enable compare match A 
  OCR1A = 312;           //Finally we set compare register A to this value ~ 50ms
  sei();

  // -------------------------------------------------------------------------------

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
//  btn2.attachClick([]() {
//    data[0] = 2;
//    data[1] = 1;
//    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
//  });
//  
//  // Double Click event attachment with lambda
//  btn2.attachDoubleClick([]() {
//    data[0] = 2;
//    data[1] = 2;
//    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
//  });
//
//  btn2.attachLongPressStart([]() {
//    data[0] = 2;
//    data[1] = 3;
//    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
//  });
//  
//  btn2.attachLongPressStop([]() {
//    data[0] = 2;
//    data[1] = 4;
//    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
//  });
}

typedef void (*functiontype)();

bool dir = true;
unsigned char pwmVal = 0;
unsigned char pwmValFrom = 0, pwmValTo = 0;

// Simply fading in and out.
void fadeInOut() {
  if (pwmVal == 1) {
    dir = true;
  } else if (pwmVal == 0xff) {
    dir = false;
  }
  
  if(dir) {
    pwmVal++;
  } else {
    pwmVal--;
  }
  
  analogWrite(PWM_LED, pwmVal);
}

void fadeTo() {
  if (pwmVal == pwmValTo) {
    TIMSK1 &= ~B00000010;    //Set OCIE1A disable interrupt  
  } else if (pwmValFrom < pwmValTo) {
    pwmVal++;
  } else {
    pwmVal--;
  }
  analogWrite(PWM_LED, pwmVal);
}

functiontype func = &fadeInOut;


//With the settings above, this IRS will trigger each 50ms.
ISR(TIMER1_COMPA_vect) {
  TCNT1  = 0;               //First, set the timer back to 0 so it resets for next interrupt
  func();
}

void loop()
{
  btn0.tick();
  btn1.tick();
//  btn2.tick();

  // ----------------- Interuption ---------------------------------
  if(!digitalRead(CAN0_INT)) {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
    // (rxId & DEVICE_ADDR) == DEVICE_ADDR)
    if((rxId & 0x40000000) == 0x40000000) {
      // Determine if message is a remote request frame.
       CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  pong);
       pong[1]++;
    } else {
       if(rxBuf[0] == 0x3) {
          if(rxBuf[1] == 0) {
            digitalWrite(PIN_RELAY, false);
          } else {
            digitalWrite(PIN_RELAY, true);
          }
          analogWrite(PWM_LED, rxBuf[1]);
       } else if(rxBuf[0] == 0x4) {
          func = &fadeInOut;
          OCR1A = rxBuf[1] << 8 | rxBuf[2]; // step
          TIMSK1 ^= B00000010;    //Toggle OCIE1A bit for enable/disable interrupt
       } else if(rxBuf[0] == 0x5) {
          pwmValFrom = rxBuf[1]; // from
          pwmValTo = rxBuf[2]; // to
          OCR1A = rxBuf[3] << 8 | rxBuf[4]; // step
          pwmVal = pwmValFrom;
          func = &fadeTo;
          TIMSK1 |= B00000010;    //Set OCIE1A enable interrupt
       }
    }
  }
  
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
