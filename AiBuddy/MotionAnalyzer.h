// MotionAnalyzer.h - turns raw IMU samples into MotionState + discrete Events.
// Knows nothing about eyes or personality.
#pragma once
#include "Types.h"

// Running statistics over the peak (maximum deviation) of each shake excursion.
struct PeakStats {
  float    sum = 0, max = 0;
  uint16_t n = 0;
  void  add(float p) { sum += p; if (p > max) max = p; n++; }
  void  reset()      { sum = 0; max = 0; n = 0; }
  float mean() const { return n ? sum / n : 0.f; }
};

class MotionAnalyzer {
 public:
  void configure(float baselineG) { baseline_ = baselineG; }
  void update(const ImuSample& s, uint32_t nowMs, EventQueue& events);
  const MotionState& state() const { return st_; }

 private:
  void trackFreeFall(uint32_t now, EventQueue& q);
  void trackImpact(uint32_t now, EventQueue& q);
  void trackShake(uint32_t now, float dt, EventQueue& q);
  void commitExcursion();

  MotionState st_;
  float baseline_ = 1.f;
  bool  restInit_ = false;

  // free fall
  bool     inFF_ = false, lowActive_ = false;
  uint32_t lowSince_ = 0, ffStart_ = 0;
  // impact after free fall
  bool     impactWatch_ = false;
  uint32_t impactUntil_ = 0;
  float    impactPeak_ = 0;
  uint32_t suppressShakeUntil_ = 0;
  // shake
  bool     wasAbove_ = false;
  uint8_t  peaks_ = 0;
  uint32_t winStart_ = 0, lastPeak_ = 0, shakeStart_ = 0;
  float    winEnergy_ = 0;
  PeakStats winPeaks_, shakePeaks_;   // per-excursion peak statistics (detection window / whole shake)
  float    excPeak_ = 0;           // highest deviation of the excursion in progress
};