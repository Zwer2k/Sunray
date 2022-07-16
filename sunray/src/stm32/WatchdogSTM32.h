// watchdog for STM32 

#ifndef WATCHDOGSTM32_H
#define WATCHDOGSTM32_H

//#include <stm32f1xx_hal_iwdg.h>

class WatchdogSTM32 {
public:
  WatchdogSTM32();

  // Enable the watchdog timer to reset the machine after a period of time
  // without any calls to reset().  The passed in period (in milliseconds)
  // is just a suggestion and a lower value might be picked if the hardware
  // does not support the exact desired value.
  // User code should NOT set the 'isForSleep' argument either way --
  // it's used internally by the library, but your sketch should leave this
  // out when calling enable(), just let the default have its way.
  //
  // The actual period (in milliseconds) before a watchdog timer reset is
  // returned.
  int enable(int maxPeriodMS = 0, bool isForSleep = false);

  // Reset or 'kick' the watchdog timer to prevent a reset of the device.
  void reset();

private:
  //IWDG_HandleTypeDef hiwdg;
};

#endif //WATCHDOGSTM32_H
