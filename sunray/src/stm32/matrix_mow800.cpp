// Adafruit Grand Central M4  (SAMD51P20A, 1024KB Flash, 256KB RAM)

// https://learn.adafruit.com/adafruit-grand-central/pinouts
// https://github.com/adafruit/ArduinoCore-samd


// FIFO size can be adjusted here (click on Arduino IDE->File->Preferences to jump into that folder):
// C:\Users\alex\AppData\Local\Arduino15\packages\adafruit\hardware\samd\1.6.0\cores\arduino\RingBuffer.h

#include "../../config.h"
#include "WatchdogSTM32.h"

#if defined(__MOW800__)

//HardwareSerial Serial1(PIN_SERIAL_RX, PIN_SERIAL_TX);
//HardwareSerial Serial2(PIN_SERIAL2_RX, PIN_SERIAL2_TX);
HardwareSerial Serial4(PD6, PD5);
//HardwareSerial Serial3(PIN_SERIAL3_RX, PIN_SERIAL3_TX);
//HardwareSerial Serial4(PC11, PC10);
//HardwareSerial Serial5(PD9, PD8);
 
WatchdogSTM32 watchdog;


void watchdogReset(){
  watchdog.reset();
}

void watchdogEnable(uint32_t timeout){
  watchdog.enable(timeout);
}


#endif   //  __MOW800__
