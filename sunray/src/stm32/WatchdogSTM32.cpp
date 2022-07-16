
// https://github.com/adafruit/Adafruit_SleepyDog/blob/master/utility/WatchdogSTM32.cpp

// Be careful to use a platform-specific conditional include to only make the
// code visible for the appropriate platform.  Arduino will try to compile and
// link all .cpp files regardless of platform.
#if defined(ARDUINO_ARCH_STM32)

#include "WatchdogSTM32.h"
#include "../../config.h"

WatchdogSTM32::WatchdogSTM32() {
  // hiwdg.Instance = IWDG;
  // hiwdg.Init.Prescaler = IWDG_PRESCALER_4;
  // hiwdg.Init.Reload = 4095;
  // if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  // {
  //   Error_Handler();
  // }
}

int WatchdogSTM32::enable(int maxPeriodMS, bool isForSleep) { 


  return 0;
}

void WatchdogSTM32::reset() {
  
}


#endif // defined(ARDUINO_ARCH_STM32)

