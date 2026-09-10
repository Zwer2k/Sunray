#pragma once

#ifdef __linux__

#include <atomic>
#include <cstdint>

struct LedStripObservation {
  bool found{false};
  float horizontalError{0.0f}; // -1 left, +1 right
  float confidence{0.0f};
  int ledCount{0};
  float verticalSpan{0.0f};    // fraction of image height
  unsigned long ageMs{0};
};

class LedStripDetector {
public:
  static LedStripDetector& instance();

  void processRgb(const uint8_t* rgb, int width, int height, unsigned long timestampMs);
  LedStripObservation observation(unsigned long nowMs) const;
  void clear();

private:
  LedStripDetector() = default;

  int acquireCount_{0}; // written only by the camera capture thread
  std::atomic<bool> found_{false};
  std::atomic<float> horizontalError_{0.0f};
  std::atomic<float> confidence_{0.0f};
  std::atomic<int> ledCount_{0};
  std::atomic<float> verticalSpan_{0.0f};
  std::atomic<unsigned long> timestampMs_{0};
};

#endif
