#include "EyePhysics.h"
#include "Config.h"

void EyePhysics::begin(uint32_t nowMs) {
  for (int i = 0; i < 2; i++) {
    body_[i].k = cfg::EYE_STIFFNESS;
    body_[i].c = cfg::EYE_DAMPING;
    body_[i].restitution = cfg::EYE_RESTITUTION;
    body_[i].limit = Vec2(cfg::LIMIT_X, cfg::LIMIT_Y);
  }
  squash_.k = cfg::SQUASH_K;
  squash_.c = cfg::SQUASH_C;
  nextBlinkMs_ = nowMs + 2000;
}

void EyePhysics::step(float dt, uint32_t nowMs, const MotionState& m, EyeIntent& in) {
  using phys::smooth;

  // 1) Ease the look parameters toward what the behaviour layer asks for.
  anger_   = smooth(anger_,   in.anger,   dt, 0.10f);
  dizzy_   = smooth(dizzy_,   in.dizzy,   dt, 0.20f);
  widen_   = smooth(widen_,   in.widen,   dt, 0.08f);
  floaty_  = smooth(floaty_,  in.floaty,  dt, 0.25f);
  tremble_ = smooth(tremble_, in.tremble, dt, 0.10f);

  // 2) Consume one-shot impulses.
  if (in.squash > 0.f) {
    squash_.pos = phys::clampf(1.f - in.squash, 0.35f, 1.f);
    squash_.vel = 0.f;
    in.squash = 0.f;
  }
  if (in.kick.x != 0.f || in.kick.y != 0.f) {
    for (int i = 0; i < 2; i++) { body_[i].vel.x += in.kick.x; body_[i].vel.y += in.kick.y; }
    in.kick = Vec2();
  }

  // 3) Forces on each eye. Gravity is the sensor's own reading: it vanishes in free fall by itself.
  t_ += dt;
  // Eyes respond to how far gravity has moved from the home pose (tilt), plus any shaking on top.
  // In weightlessness there is no home pose to compare against, so it fades out with floaty_.
  const float homeW = 1.f - floaty_;
  const float gx = (m.gravity.x - m.restGravity.x * homeW) * cfg::GRAVITY_GAIN;
  const float gy = (m.gravity.y - m.restGravity.y * homeW) * cfg::GRAVITY_GAIN;
  const float drift = cfg::FLOAT_DRIFT_ACCEL * floaty_;
  const float shiver = cfg::TREMBLE_ACCEL * tremble_;
  const float kFloat = 1.f - 0.75f * floaty_;   // weightless = soft, slow springs
  const float cFloat = 1.f - 0.50f * floaty_;

  for (int i = 0; i < 2; i++) {
    const float ph = t_ + 0.7f * i;             // each eye drifts slightly out of step
    Vec2 a(gx + drift * sinf(ph * 1.3f) + shiver * (random(-100, 101) / 100.f),
           gy + drift * cosf(ph * 0.9f + 1.f)  + shiver * (random(-100, 101) / 100.f));
    const float kSpread = (i == 0) ? 0.95f : 1.05f;  // slightly different "weight" = organic motion
    body_[i].step(dt, a, kFloat * kSpread, cFloat);
  }

  squash_.step(dt);
  dizzyPhase_ += dt * cfg::DIZZY_SPIN_RAD_S * (0.4f + 0.6f * dizzy_);
  if (dizzyPhase_ > 6.2831853f) dizzyPhase_ -= 6.2831853f;
  updateBlink(dt, nowMs, in.autoBlink);

  // 4) Compose the frame. Squash keeps volume: flatter eyes get wider.
  const float sy = squash_.pos;
  const float sx = 1.f + (1.f - sy) * 0.6f;
  const float open = openness_ < 0.07f ? 0.07f : openness_;
  frame_.scale  = widen_;
  frame_.w      = cfg::EYE_W * widen_ * sx;
  frame_.h      = cfg::EYE_H * widen_ * sy * open;
  frame_.radius = fminf(cfg::EYE_RADIUS, frame_.h * 0.5f);
  const float amp = cfg::DIZZY_ORBIT_PX * dizzy_;
  for (int i = 0; i < 2; i++) {
    const float p = dizzyPhase_ + 1.6f * i;
    frame_.pos[i] = Vec2(body_[i].pos.x + cosf(p) * amp, body_[i].pos.y + sinf(p) * amp);
  }
  frame_.anger = anger_;
  frame_.dizzy = dizzy_;
  frame_.dizzyPhase = dizzyPhase_;
}

void EyePhysics::updateBlink(float dt, uint32_t nowMs, bool enabled) {
  if (!blinking_) {
    openness_ = 1.f;
    if (enabled && nowMs >= nextBlinkMs_) { blinking_ = true; blinkT_ = 0; }
    return;
  }
  blinkT_ += dt;
  const float p = blinkT_ / cfg::BLINK_SECONDS;
  if (p >= 1.f) {
    blinking_ = false; openness_ = 1.f;
    nextBlinkMs_ = nowMs + random((long)cfg::BLINK_MIN_MS, (long)cfg::BLINK_MAX_MS);
    return;
  }
  openness_ = (p < 0.4f) ? 1.f - p / 0.4f : (p - 0.4f) / 0.6f;  // fast close, slower open
}
