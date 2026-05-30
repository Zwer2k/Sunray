// Ardumower Sunray
// Copyright (c) 2013-2020 by Alexander Grau, Grau GmbH
// Licensed GPLv3 for open source use
// or Grau GmbH Commercial License for commercial use (http://grauonline.de/cms2/?page_id=153)


#include "sonar.h"
#include "config.h"
#include "RunningMedian.h"
#include <Arduino.h>

#if defined(SONAR_INSTALLED)
#if !defined(pinSonarLeftTrigger) || !defined(pinSonarLeftEcho) || \
    !defined(pinSonarCenterTrigger) || !defined(pinSonarCenterEcho) || \
    !defined(pinSonarRightTrigger) || !defined(pinSonarRightEcho)
#undef SONAR_INSTALLED
#endif
#endif


#define MAX_DURATION 4000
#define ROUNDING_ENABLED false
#define US_ROUNDTRIP_CM 57      // Microseconds (uS) it takes sound to travel round-trip 1cm (2cm total), uses integer to save compiled code space. Default=57

// Conversion from uS to distance (round result to nearest cm or inch).
#define NewPingConvert(echoTime, conversionFactor) (max(((unsigned int)echoTime + conversionFactor / 2) / conversionFactor, (echoTime ? 1 : 0)))

#ifndef SONAR_MEDIAN_SAMPLES
  #define SONAR_MEDIAN_SAMPLES 9
#endif
#ifndef SONAR_EVAL_INTERVAL_MS
  #define SONAR_EVAL_INTERVAL_MS 50
#endif
#ifndef SONAR_DEBOUNCE_US
  #define SONAR_DEBOUNCE_US 0
#endif

RunningMedian<unsigned int, SONAR_MEDIAN_SAMPLES> sonarLeftMeasurements;
RunningMedian<unsigned int, SONAR_MEDIAN_SAMPLES> sonarRightMeasurements;
RunningMedian<unsigned int, SONAR_MEDIAN_SAMPLES> sonarCenterMeasurements;

volatile unsigned long startTime[3] = {0, 0, 0};
volatile unsigned long echoDuration[3] = {0, 0, 0};
volatile byte sonarIdx = 0;
bool added = false;
unsigned long timeoutTime = 0;
unsigned long nextEvalTime = 0;
#ifdef SONAR_PARALLEL_TRIGGER
  bool triggered[3] = {false, false, false};
#endif


#ifdef SONAR_INSTALLED

// HC-SR04 ultrasonic sensor driver (2cm - 400cm)
void startHCSR04(int triggerPin) {
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
}

static bool debounce(unsigned long &lastTime, unsigned long now) {
  if (now - lastTime < SONAR_DEBOUNCE_US) return false;
  lastTime = now;
  return true;
}

void echoLeft() {
#ifndef SONAR_PARALLEL_TRIGGER
  if (sonarIdx != 0) return;
#endif
  unsigned long now = micros();
  static unsigned long lastInt = 0;
  if (SONAR_DEBOUNCE_US > 0 && !debounce(lastInt, now)) return;
  if (digitalRead(pinSonarLeftEcho) == HIGH) {
    startTime[0] = now;
  } else {
    if (now >= startTime[0]) echoDuration[0] = now - startTime[0];
  }
}

void echoCenter() {
#ifndef SONAR_PARALLEL_TRIGGER
  if (sonarIdx != 1) return;
#endif
  unsigned long now = micros();
  static unsigned long lastInt = 0;
  if (SONAR_DEBOUNCE_US > 0 && !debounce(lastInt, now)) return;
  if (digitalRead(pinSonarCenterEcho) == HIGH) {
    startTime[1] = now;
  } else {
    if (now >= startTime[1]) echoDuration[1] = now - startTime[1];
  }
}

void echoRight() {
#ifndef SONAR_PARALLEL_TRIGGER
  if (sonarIdx != 2) return;
#endif
  unsigned long now = micros();
  static unsigned long lastInt = 0;
  if (SONAR_DEBOUNCE_US > 0 && !debounce(lastInt, now)) return;
  if (digitalRead(pinSonarRightEcho) == HIGH) {
    startTime[2] = now;
  } else {
    if (now >= startTime[2]) echoDuration[2] = now - startTime[2];
  }
}

#endif


void Sonar::run() {
#ifdef SONAR_INSTALLED  
  if (!enabled) {
    distanceRight = distanceLeft = distanceCenter = 0;
    return;
  }

#ifdef SONAR_PARALLEL_TRIGGER
  for (int i = 0; i < 3; i++) {
    if (echoDuration[i] != 0) {
      unsigned long raw = echoDuration[i];
      if (raw > MAX_DURATION) raw = MAX_DURATION;
      if (i == 0) sonarLeftMeasurements.add(raw);
      else if (i == 1) sonarCenterMeasurements.add(raw);
      else sonarRightMeasurements.add(raw);
      echoDuration[i] = 0;
      triggered[i] = true;
    }
  }

  if (millis() > timeoutTime) {
    for (int i = 0; i < 3; i++) {
      if (!triggered[i]) {
        if (i == 0) sonarLeftMeasurements.add(MAX_DURATION);
        else if (i == 1) sonarCenterMeasurements.add(MAX_DURATION);
        else sonarRightMeasurements.add(MAX_DURATION);
      }
      triggered[i] = false;
    }
    startHCSR04(pinSonarLeftTrigger);
    startHCSR04(pinSonarCenterTrigger);
    startHCSR04(pinSonarRightTrigger);
    timeoutTime = millis() + 50;
  }
#else
  if (echoDuration[sonarIdx] != 0) {
    added = true;
    unsigned long raw = echoDuration[sonarIdx];
    if (raw > MAX_DURATION) raw = MAX_DURATION;
    if (sonarIdx == 0) sonarLeftMeasurements.add(raw);
    else if (sonarIdx == 1) sonarCenterMeasurements.add(raw);
    else sonarRightMeasurements.add(raw);
    echoDuration[sonarIdx] = 0;
  }
  if (millis() > timeoutTime) {
    if (!added) {
      if (sonarIdx == 0) sonarLeftMeasurements.add(MAX_DURATION);
      else if (sonarIdx == 1) sonarCenterMeasurements.add(MAX_DURATION);
      else sonarRightMeasurements.add(MAX_DURATION);
    }
    sonarIdx = (sonarIdx + 1) % 3;
    echoDuration[sonarIdx] = 0;
    if (sonarIdx == 0) startHCSR04(pinSonarLeftTrigger);
    else if (sonarIdx == 1) startHCSR04(pinSonarCenterTrigger);
    else startHCSR04(pinSonarRightTrigger);
    timeoutTime = millis() + 50;
    added = false;
  }
#endif

  if (millis() > nextEvalTime) {
    nextEvalTime = millis() + SONAR_EVAL_INTERVAL_MS;
    float value;
    sonarLeftMeasurements.getMedian(distanceLeft);
    distanceLeft = convertCm(distanceLeft);
    sonarRightMeasurements.getMedian(distanceRight);
    distanceRight = convertCm(distanceRight);
    sonarCenterMeasurements.getMedian(distanceCenter);
    distanceCenter = convertCm(distanceCenter);
  }
#endif
}

void Sonar::begin()
{
#ifdef SONAR_INSTALLED
  enabled = SONAR_ENABLE;
  triggerLeftBelow = SONAR_LEFT_OBSTACLE_CM;
  triggerCenterBelow = SONAR_CENTER_OBSTACLE_CM;
  triggerRightBelow = SONAR_RIGHT_OBSTACLE_CM;
  pinMode(pinSonarLeftTrigger , OUTPUT);
  pinMode(pinSonarCenterTrigger , OUTPUT);
  pinMode(pinSonarRightTrigger , OUTPUT);

  pinMode(pinSonarLeftEcho , INPUT);
  pinMode(pinSonarCenterEcho , INPUT);
  pinMode(pinSonarRightEcho , INPUT);

  attachInterrupt(digitalPinToInterrupt(pinSonarLeftEcho), echoLeft, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinSonarCenterEcho), echoCenter, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinSonarRightEcho), echoRight, CHANGE);

  verboseOutput = false;
  nearObstacleTimeout = 0;
#endif
}


bool Sonar::obstacle()
{
#ifdef SONAR_INSTALLED
  if (!enabled) return false;
  return ((distanceLeft < triggerLeftBelow) || (distanceCenter < triggerCenterBelow) || (distanceRight < triggerRightBelow));
#else
  return false;
#endif
}

bool Sonar::nearObstacle()
{
#ifdef SONAR_INSTALLED
  if (!enabled) return false;
  int nearZone = 30; // cm
  if ((nearObstacleTimeout != 0) && (millis() < nearObstacleTimeout)) return true;
  nearObstacleTimeout = 0;
  bool res = ((distanceLeft < triggerLeftBelow + nearZone) || (distanceCenter < triggerCenterBelow + nearZone) || (distanceRight < triggerRightBelow + nearZone));
  if (res) {
    nearObstacleTimeout = millis() + 5000;
  }
  return res;
#else
  return false;
#endif
}

unsigned int Sonar::convertCm(unsigned int echoTime) {
#if ROUNDING_ENABLED == false
  return (echoTime / US_ROUNDTRIP_CM);              // Convert uS to centimeters (no rounding).
#else
  return NewPingConvert(echoTime, US_ROUNDTRIP_CM); // Convert uS to centimeters.
#endif
}
