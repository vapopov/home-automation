// CAN Send Example
//

#include <mcp_can.h>
#include <SPI.h>
#include <OneButton.h>

#define DEVICE_ADDR      0x109
#define CAN_FILTER_MASK 0x010F0000
#define CAN_FILTER      0x01090000

MCP_CAN CAN0(10);     // Set CS to pin 10
#define CAN0_INT 9    // Set INT to pin 9

#define PWM_LED 3 // PD3, OC2B
#define PIN_LED_RELAY 6
#define PIN_LED_RELAY2 7

#define BUTTON_ZERO 4
#define BUTTON_ONE 5


// To active CLK0 output from avr ************
// avrdude -c usbasp -p m328p -U lfuse:w:0xB7:m
// http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p&LOW=B7&HIGH=DA&EXTENDED=FD&LOCKBIT=FF  

byte data[8] = {0x00, 0x00};
//             -addr--inc-  -volt-----  -curr-----  -flg-
byte pong[7] = {0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];

/********************************************************************/

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
  pinMode(PWM_LED, OUTPUT);
  pinMode(PIN_LED_RELAY, OUTPUT);
  pinMode(PIN_LED_RELAY2, OUTPUT);
    
  // ------------------------------ Set Timers -------------------------------------
  cli();
  TCCR1A = 0;             //Reset entire TCCR1A register
  TCCR1B = 0;             //Reset entire TCCR1B register
  TCCR1B |= B00000100;    //Set CS12 to 1 so we get Prescalar = 256
  TCNT1 = 0;              //Reset Timer 1 value to 0
  // TIMSK1 |= B00000010; //Set OCIE1A to 1 so we enable compare match A 
  OCR1A = 312;            //Finally we set compare register A to this value ~ 5ms
  
  // +++++++++ timer 2 ++++++++++++++++++++
  /*set fast PWM mode with non-inverted output*/ // Пины D3 и D11 - 62.5 кГц
  TCCR2A = (0 << WGM02) | (1 << WGM01) | (1 << WGM00);
  // Normal port operation, OC2A disconnected.
  TCCR2A |= (0 << COM2A1) | (0 << COM2A0); 
  // Normal port operation, OC2B disconnected
  TCCR2A |= (0 << COM2B1) | (0 << COM2B0); 
  // B Register +++++++++++++++++++++++++++
  TCCR2B = (0 << CS22) | (0 << CS21) | (1 << CS20); // no prescaller
 
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
}

typedef void (*functiontype)();

bool dir = true;
unsigned char pwmVal = 0;
unsigned char pwmValFrom = 0, pwmValTo = 0;

// Simply fading in and out.
void fadeInOut() {
  if (pwmVal == pwmValFrom) {
    dir = pwmValFrom < pwmValTo ;
  } else if (pwmVal == pwmValTo) {
    dir = pwmValFrom > pwmValTo;
  }
  if(dir) {
    pwmVal++;
  } else {
    pwmVal--;
  }
  analogWrite(PWM_LED, pwmVal);
}

// Fading from to specific value by timer with delay info.
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
  TCNT1  = 0;  //First, set the timer back to 0 so it resets for next interrupt
  func();
}

// ----------------------------------------- Main Loop --------------------------------------------
void loop()
{   
  btn0.tick();
  btn1.tick();

  if(!digitalRead(CAN0_INT)) {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
    if((rxId & 0x40000000) == 0x40000000) {
       CAN0.sendMsgBuf(DEVICE_ADDR, 0, 7,  pong);
       pong[1]++;
    } else {
       if(rxBuf[0] == 0x3) {
          TIMSK1 &= ~B00000010;             //Set OCIE1A disable interrupt  
          pwmVal = rxBuf[1];
          analogWrite(PWM_LED, pwmVal);
       } else if(rxBuf[0] == 0x4) {
          func = &fadeInOut;
          pwmValFrom = rxBuf[1];            // from
          pwmValTo = rxBuf[2];              // to
          OCR1A = rxBuf[3] << 8 | rxBuf[4]; // step
          pwmVal = pwmValFrom;
          TIMSK1 |= B00000010;              //Set OCIE1A enable interrupt
          analogWrite(PWM_LED, pwmVal);
       } else if(rxBuf[0] == 0x5) {
          func = &fadeTo;
          pwmValFrom = rxBuf[1];            // from
          pwmValTo = rxBuf[2];              // to
          OCR1A = rxBuf[3] << 8 | rxBuf[4]; // step
          pwmVal = pwmValFrom;
          TIMSK1 |= B00000010;              //Set OCIE1A enable interrupt
       } else if(rxBuf[0] == 0x6) {
          if (rxBuf[1] > 0) {
            digitalWrite(PIN_LED_RELAY2, true);
          } else {
            digitalWrite(PIN_LED_RELAY2, false);
          }
       }

       if(pwmVal == 0) {
         digitalWrite(PIN_LED_RELAY, false);
       } else {
         digitalWrite(PIN_LED_RELAY, true);
       }
    }
  }
}


/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
