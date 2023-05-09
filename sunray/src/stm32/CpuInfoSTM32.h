#if defined(ARDUINO_ARCH_STM32)

#ifndef CPUINFOSTM32_H
#define CPUINFOSTM32_H

float readVdd();
float readTempSensor(float Vdd);

#endif // CPUINFOSTM32_H

#endif // ARDUINO_ARCH_STM32
