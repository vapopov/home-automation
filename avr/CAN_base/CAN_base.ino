// CAN Send Example
//

//#include <mcp_can.h>
//#include <SPI.h>
//#include <OneButton.h>

#include <OneWire.h> 

// #define DEVICE_ADDR     0x109
// #define CAN_FILTER_MASK 0x010F0000
// #define CAN_FILTER      0x01090000

//MCP_CAN CAN0(10);     // Set CS to pin 10
#define CAN0_INT 9    // Set INT to pin 9
#define ONE_WIRE_BUS 2

// To active CLK0 output from avr ************
// avrdude -c usbasp -p m328p -U lfuse:w:0xB7:m
// http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p&LOW=B7&HIGH=DA&EXTENDED=FD&LOCKBIT=FF  



/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
//OneWire ds(ONE_WIRE_BUS); 
/********************************************************************/

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

uint8_t i;

void setup()
{
  Serial.begin(115200);

  // turn on SPI in slave mode
  SPCR |= _BV(SPE);
  PORTB = B00000000;

  // Initialize MCP2515 running at 8MHz with a baudrate of 125kb/s and the masks and filters disabled.
//   if(CAN0.begin(MCP_STD, CAN_125KBPS, MCP_16MHZ) == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
//   else Serial.println("Error Initializing MCP2515...");

//   CAN0.init_Mask(0,0,CAN_FILTER_MASK);           // Init first mask...
//   CAN0.init_Filt(0,0,CAN_FILTER);                // Init second filter...
//   CAN0.init_Filt(1,0,CAN_FILTER);                // Init second filter...
//
//   CAN0.init_Mask(1,0,CAN_FILTER_MASK);
//   CAN0.init_Filt(2,0,CAN_FILTER);
//   CAN0.init_Filt(3,0,CAN_FILTER);
//   CAN0.init_Filt(4,0,CAN_FILTER);
//   CAN0.init_Filt(5,0,CAN_FILTER);
//
//   CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted

//   pinMode(CAN0_INT, INPUT);
//   pinMode(PWM_LED, OUTPUT);
//   pinMode(PIN_LED_RELAY, OUTPUT);
//   pinMode(PIN_HEATER_RELAY, OUTPUT);
}

// ----------------------------------------- Main Loop --------------------------------------------
void loop()
{
  delay(1000);
  i++;
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
