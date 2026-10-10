#ifdef __linux__

#include "LedStripDetector.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
#include "../../config.h"

// Arduino compatibility headers define these as macros. They break the C++
// standard-library overloads used by the native Linux detector.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#ifndef DOCK_LED_STRIP_MIN_BRIGHTNESS
#define DOCK_LED_STRIP_MIN_BRIGHTNESS 220
#endif
#ifndef DOCK_LED_STRIP_MAX_COLOR_SPREAD
#define DOCK_LED_STRIP_MAX_COLOR_SPREAD 60
#endif
#ifndef DOCK_LED_STRIP_MIN_LEDS
#define DOCK_LED_STRIP_MIN_LEDS 5
#endif
#ifndef DOCK_LED_STRIP_MIN_VERTICAL_SPAN
#define DOCK_LED_STRIP_MIN_VERTICAL_SPAN 0.18f
#endif
#ifndef DOCK_LED_STRIP_MAX_SLOPE
#define DOCK_LED_STRIP_MAX_SLOPE 0.12f
#endif
#ifndef DOCK_LED_STRIP_LOST_TIMEOUT_MS
#define DOCK_LED_STRIP_LOST_TIMEOUT_MS 500
#endif
#ifndef DOCK_LED_STRIP_ACQUIRE_FRAMES
#define DOCK_LED_STRIP_ACQUIRE_FRAMES 3
#endif

namespace {

struct BrightSpot {
  float x;
  float y;
  int area;
};

static bool isLedPixel(const uint8_t* pixel) {
  const int lo = std::min((int)pixel[0], std::min((int)pixel[1], (int)pixel[2]));
  const int hi = std::max((int)pixel[0], std::max((int)pixel[1], (int)pixel[2]));
  return hi >= DOCK_LED_STRIP_MIN_BRIGHTNESS &&
         lo >= DOCK_LED_STRIP_MIN_BRIGHTNESS &&
         hi - lo <= DOCK_LED_STRIP_MAX_COLOR_SPREAD;
}

} // namespace

LedStripDetector& LedStripDetector::instance() {
  static LedStripDetector detector;
  return detector;
}

void LedStripDetector::clear() {
  acquireCount_ = 0;
  found_.store(false, std::memory_order_release);
  confidence_.store(0.0f, std::memory_order_relaxed);
  ledCount_.store(0, std::memory_order_relaxed);
  verticalSpan_.store(0.0f, std::memory_order_relaxed);
  timestampMs_.store(0, std::memory_order_relaxed);
}

void LedStripDetector::processRgb(const uint8_t* rgb, int width, int height,
                                  unsigned long timestampMs) {
  if (!rgb || width < 16 || height < 16) {
    clear();
    return;
  }

  // Work on a small image. This keeps the detector inexpensive even when the
  // cloud requests a 1080p camera stream.
  const int workW = std::min(width, 320);
  const int workH = std::max(1, height * workW / width);
  const int pixelCount = workW * workH;
  std::vector<uint8_t> mask((size_t)pixelCount, 0);
  std::vector<uint8_t> visited((size_t)pixelCount, 0);

  for (int y = 0; y < workH; y++) {
    const int srcY = y * height / workH;
    for (int x = 0; x < workW; x++) {
      const int srcX = x * width / workW;
      const uint8_t* pixel = rgb + ((size_t)srcY * width + srcX) * 3;
      if (isLedPixel(pixel)) mask[(size_t)y * workW + x] = 1;
    }
  }

  std::vector<BrightSpot> spots;
  std::vector<int> stack;
  stack.reserve(128);
  for (int start = 0; start < pixelCount; start++) {
    if (!mask[(size_t)start] || visited[(size_t)start]) continue;
    stack.clear();
    stack.push_back(start);
    visited[(size_t)start] = 1;
    int area = 0;
    int minX = workW, maxX = 0, minY = workH, maxY = 0;
    long sumX = 0, sumY = 0;
    while (!stack.empty()) {
      const int pos = stack.back();
      stack.pop_back();
      const int x = pos % workW;
      const int y = pos / workW;
      area++;
      sumX += x;
      sumY += y;
      minX = std::min(minX, x); maxX = std::max(maxX, x);
      minY = std::min(minY, y); maxY = std::max(maxY, y);
      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
          if (dx == 0 && dy == 0) continue;
          const int nx = x + dx;
          const int ny = y + dy;
          if (nx < 0 || nx >= workW || ny < 0 || ny >= workH) continue;
          const int next = ny * workW + nx;
          if (mask[(size_t)next] && !visited[(size_t)next]) {
            visited[(size_t)next] = 1;
            stack.push_back(next);
          }
        }
      }
    }

    const int boxW = maxX - minX + 1;
    const int boxH = maxY - minY + 1;
    // Individual LEDs remain compact. Large white walls, windows and floor
    // reflections are deliberately rejected here.
    if (area >= 1 && area <= 120 && boxW <= 16 && boxH <= 16 &&
        boxW <= boxH * 4 && boxH <= boxW * 4) {
      spots.push_back({(float)sumX / area, (float)sumY / area, area});
    }
  }

  int bestCount = 0;
  float bestSpan = 0.0f;
  float bestResidual = 1e9f;
  float bestA = 0.0f;
  float bestB = 0.0f;
  float bestMinY = 0.0f;
  float bestMaxY = 0.0f;
  const float tolerance = std::max(2.5f, workW * 0.012f);
  const float minPairSpan = workH * 0.10f;
  const float maxLedGap = std::max(8.0f, workH * 0.08f);

  // Fit a near-vertical line through every plausible spot pair and keep the
  // model supported by the largest vertically distributed LED chain.
  for (size_t i = 0; i < spots.size(); i++) {
    for (size_t j = i + 1; j < spots.size(); j++) {
      const float dy = spots[j].y - spots[i].y;
      if (std::fabs(dy) < minPairSpan) continue;
      const float a = (spots[j].x - spots[i].x) / dy;
      if (std::fabs(a) > DOCK_LED_STRIP_MAX_SLOPE) continue;
      const float b = spots[i].x - a * spots[i].y;
      std::vector<const BrightSpot*> support;
      for (const BrightSpot& spot : spots) {
        const float error = std::fabs(spot.x - (a * spot.y + b));
        if (error <= tolerance) support.push_back(&spot);
      }
      std::sort(support.begin(), support.end(), [](const BrightSpot* lhs, const BrightSpot* rhs) {
        return lhs->y < rhs->y;
      });

      int chainStart = 0;
      for (int end = 0; end < (int)support.size(); end++) {
        if (end > 0 && support[(size_t)end]->y - support[(size_t)end - 1]->y > maxLedGap) {
          chainStart = end;
        }
        const int count = end - chainStart + 1;
        const float minSpotY = support[(size_t)chainStart]->y;
        const float maxSpotY = support[(size_t)end]->y;
        const float span = (maxSpotY - minSpotY) / workH;
        float residual = 0.0f;
        for (int k = chainStart; k <= end; k++) {
          const BrightSpot* spot = support[(size_t)k];
          residual += std::fabs(spot->x - (a * spot->y + b));
        }
        if (count > bestCount ||
            (count == bestCount && span > bestSpan) ||
            (count == bestCount && std::fabs(span - bestSpan) < 0.001f && residual < bestResidual)) {
          bestCount = count;
          bestSpan = span;
          bestResidual = residual;
          bestA = a;
          bestB = b;
          bestMinY = minSpotY;
          bestMaxY = maxSpotY;
        }
      }
    }
  }

  const bool found = bestCount >= DOCK_LED_STRIP_MIN_LEDS &&
                     bestSpan >= DOCK_LED_STRIP_MIN_VERTICAL_SPAN;
#ifdef DOCK_LED_STRIP_DEBUG
  std::fprintf(stderr, "LED model spots=%zu count=%d span=%.3f a=%.3f b=%.3f residual=%.3f\n",
               spots.size(), bestCount, bestSpan, bestA, bestB, bestResidual);
  for (const BrightSpot& spot : spots) {
    const float error = std::fabs(spot.x - (bestA * spot.y + bestB));
    if (error <= tolerance && spot.y >= bestMinY && spot.y <= bestMaxY) {
      std::fprintf(stderr, "  support x=%.2f y=%.2f area=%d error=%.2f\n",
                   spot.x, spot.y, spot.area, error);
    }
  }
#endif
  if (!found) {
    acquireCount_ = 0;
    found_.store(false, std::memory_order_release);
    confidence_.store(0.0f, std::memory_order_relaxed);
    ledCount_.store(bestCount, std::memory_order_relaxed);
    verticalSpan_.store(bestSpan, std::memory_order_relaxed);
    timestampMs_.store(timestampMs, std::memory_order_relaxed);
    return;
  }

  // Aim at the observed chain itself. Extrapolating to the image centre makes
  // a short strip near the image edge unnecessarily sensitive to line slope.
  const float xAtCenter = bestA * ((bestMinY + bestMaxY) * 0.5f) + bestB;
  const float horizontalError = std::max(-1.0f, std::min(1.0f,
      (xAtCenter - workW * 0.5f) / (workW * 0.5f)));
  const float countScore = std::min(1.0f, bestCount / 10.0f);
  const float spanScore = std::min(1.0f, bestSpan / 0.45f);
  const float confidence = 0.6f * countScore + 0.4f * spanScore;

  if (acquireCount_ < DOCK_LED_STRIP_ACQUIRE_FRAMES) acquireCount_++;

  horizontalError_.store(horizontalError, std::memory_order_relaxed);
  confidence_.store(confidence, std::memory_order_relaxed);
  ledCount_.store(bestCount, std::memory_order_relaxed);
  verticalSpan_.store(bestSpan, std::memory_order_relaxed);
  timestampMs_.store(timestampMs, std::memory_order_relaxed);
  found_.store(acquireCount_ >= DOCK_LED_STRIP_ACQUIRE_FRAMES, std::memory_order_release);
}

LedStripObservation LedStripDetector::observation(unsigned long nowMs) const {
  LedStripObservation result;
  const unsigned long timestamp = timestampMs_.load(std::memory_order_relaxed);
  result.ageMs = timestamp ? nowMs - timestamp : (unsigned long)-1;
  result.horizontalError = horizontalError_.load(std::memory_order_relaxed);
  result.confidence = confidence_.load(std::memory_order_relaxed);
  result.ledCount = ledCount_.load(std::memory_order_relaxed);
  result.verticalSpan = verticalSpan_.load(std::memory_order_relaxed);
  result.found = found_.load(std::memory_order_acquire) &&
                 timestamp && result.ageMs <= DOCK_LED_STRIP_LOST_TIMEOUT_MS;
  return result;
}

#endif
