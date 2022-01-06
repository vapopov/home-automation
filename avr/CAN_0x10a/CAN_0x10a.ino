// CAN Send Example
//

#include <mcp_can.h>
#include <SPI.h>
#include <OneButton.h>


#define DEVICE_ADDR     0x10a
#define CAN_FILTER_MASK 0x010F0000
#define CAN_FILTER      0x010a0000

MCP_CAN CAN0(10);     // Set CS to pin 10
#define CAN0_INT 9    // Set INT to pin 9

#define BUTTON_ZERO  4
#define BUTTON_ONE   5
#define BUTTON_TWO   6
#define BUTTON_THREE 14

#define CLK 2                         // Указываем к какому выводу CLK энкодер подключен к Arduino
#define DT 3                          // Указываем к какому выводу DT энкодер подключен к Arduino
#define SW 4                          // Указываем к какому выводу SW энкодер подключен к Arduino

int counter = 0;                      // Создаем переменную counter
int currentStateCLK;                  // Создаем переменную currentStateCLK
int lastStateCLK;                     // Создаем переменную lastStateCLK
unsigned long lastButtonPress = 0;    // Создаем переменную lastBut

// To active CLK0 output from avr ************
// avrdude -c usbasp -p m328p -U lfuse:w:0xB7:m
// http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p&LOW=B7&HIGH=DA&EXTENDED=FD&LOCKBIT=FF  

byte data[8] = {0x00, 0x00};
//             -addr--inc-  -temp-----  -set temp-  -gap-
byte pong[7] = {0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];

long lastUpdateTime = 0; // Last time when check was processed.
const int TEMP_UPDATE_TIME = 1000; // Period in ms for checking temperature.


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
 BUTTON_TWO,   // Input pin for the button
 true,         // Button is active LOW
 true          // Enable internal pull-up resistor
);

OneButton btn3 = OneButton(
 BUTTON_THREE,   // Input pin for the button
 true,         // Button is active LOW
 true          // Enable internal pull-up resistor
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

  pinMode(CLK, INPUT);                // Указываем вывод CLK как вход
  pinMode(DT, INPUT);                 // Указываем вывод DT как вход
  pinMode(SW, INPUT_PULLUP);          // Указываем вывод SW как вход и включаем подтягивающий резистор
  lastStateCLK = digitalRead(CLK);    // Считываем значение с CLK


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

  // ------------------------------------------------
  // Single Click event attachment
  btn3.attachClick([]() {
    data[0] = 3;
    data[1] = 1;
    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
  });

  // Double Click event attachment with lambda
  btn3.attachDoubleClick([]() {
    data[0] = 3;
    data[1] = 2;
    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
  });

  btn3.attachLongPressStart([]() {
    data[0] = 3;
    data[1] = 3;
    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
  });

  btn3.attachLongPressStop([]() {
    data[0] = 3;
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
  btn3.tick();

  currentStateCLK = digitalRead(CLK); // Считываем значение с CLK
  if (currentStateCLK != lastStateCLK  && currentStateCLK == 1) {
    if (digitalRead(DT) != currentStateCLK) {
      counter --;
      data[2] = 0;
    } else {
      counter ++;
      data[2] = 1;
    }
    data[0] = 4;
    data[1] = 2;
    data[3] = counter;
    CAN0.sendMsgBuf(DEVICE_ADDR, 0, 4,  data);
  }

  lastStateCLK = currentStateCLK;         // Запопоследнее состояние CLK
  int btnState = digitalRead(SW);         // Считываем состояние вывода SW

  if (btnState == LOW) {                   // Если состояние LOW, кнопка нажата
    if (millis() - lastButtonPress > 50) { // Если состояние LOW в течении 50 мкс, кнопка нажата
      data[0] = 4;
      data[1] = 1;
      CAN0.sendMsgBuf(DEVICE_ADDR, 0, 2,  data);
    }
    lastButtonPress = millis();
  }

  if(!digitalRead(CAN0_INT)) {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
    // (rxId & DEVICE_ADDR) == DEVICE_ADDR)
    if((rxId & 0x40000000) == 0x40000000) {
      // Determine if message is a remote request frame.
       pong[2] = counter;
       CAN0.sendMsgBuf(DEVICE_ADDR, 0, 7,  pong);
       pong[1]++;
    } else {
//        if(rxBuf[0] == 0x3) {
//           TIMSK1 &= ~B00000010;             //Set OCIE1A disable interrupt
//           pwmVal = rxBuf[1];
//           analogWrite(PWM_LED, pwmVal);
//        }
    }
  }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
