// MotionAnalyzer.h - turns raw IMU samples into MotionState + discrete Events.
// Knows nothing about eyes or personality.
#pragma once
#include "Types.h"

class MotionAnalyzer {
 public:
  void configure(float baselineG) { baseline_ = baselineG; }
  void update(const ImuSample& s, uint32_t nowMs, EventQueue& events);
  const MotionState& state() const { return st_; }

 private:
  void trackFreeFall(uint32_t now, EventQueue& q);
  void trackImpact(uint32_t now, EventQueue& q);
  void trackShake(uint32_t now, float dt, EventQueue& q);

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
};
