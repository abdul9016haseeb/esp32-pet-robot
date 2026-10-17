// EyePhysics.h - turns motion + intent into eye geometry. Pure simulation, no drawing.
#pragma once
#include "Types.h"
#include "Physics.h"

class EyePhysics {
 public:
  void begin(uint32_t nowMs);
  void step(float dt, uint32_t nowMs, const MotionState& motion, EyeIntent& intent);
  const EyeFrame& frame() const { return frame_; }

 private:
  void updateBlink(float dt, uint32_t nowMs, bool enabled);

  phys::Body2D   body_[2];
  phys::Spring1D squash_;
  EyeFrame       frame_;
  float anger_ = 0, dizzy_ = 0, widen_ = 1, floaty_ = 0, tremble_ = 0;
  float dizzyPhase_ = 0, t_ = 0;
  float openness_ = 1, blinkT_ = 0;
  bool  blinking_ = false;
  uint32_t nextBlinkMs_ = 0;
};
