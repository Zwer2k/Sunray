// Adafruit Grand Central M4  (SAMD51P20A, 1024KB Flash, 256KB RAM)


#if defined(__MOW800__)

#ifndef MATRIX_MOW800
#define MATRIX_MOW800

// extern HardwareSerial Serial2;
// extern HardwareSerial Serial3;

extern HardwareSerial Serial4;
extern HardwareSerial Serial5;

extern void watchdogReset();
extern void watchdogEnable(uint32_t timeout);

#endif  // MATRIX_MOW800

#endif  // __MOW800__

