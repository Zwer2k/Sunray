// watchdog for STM32 
#if defined(ARDUINO_ARCH_STM32)

#ifndef WATCHDOGSTM32_H
#define WATCHDOGSTM32_H

#include "Arduino.h"

#if !defined(IWDG) && defined(IWDG1)
  #define IWDG IWDG1
#endif

// Minimal timeout in microseconds
#define IWDG_TIMEOUT_MIN    ((4*1000000)/LSI_VALUE)
// Maximal timeout in microseconds
#define IWDG_TIMEOUT_MAX    (((256*1000000)/LSI_VALUE)*IWDG_RLR_RL)

#define IS_IWDG_TIMEOUT(X)  (((X) >= IWDG_TIMEOUT_MIN) &&\
                             ((X) <= IWDG_TIMEOUT_MAX))

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
  void begin(uint32_t timeout, uint32_t window = IWDG_TIMEOUT_MAX);
  
  void set(uint32_t timeout, uint32_t window = IWDG_TIMEOUT_MAX);
  void get(uint32_t *timeout, uint32_t *window = NULL);
  
  void reload(void);

  bool isEnabled(void)
  {
    return _enabled;
  };
  bool isReset(bool clear = false);
  void clearReset(void);
private:
  static bool _enabled;
};

#endif //WATCHDOGSTM32_H

#endif //ARDUINO_ARCH_STM32
