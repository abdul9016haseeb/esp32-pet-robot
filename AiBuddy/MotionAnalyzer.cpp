#include "MotionAnalyzer.h"
#include "Config.h"
#include "Physics.h"

void MotionAnalyzer::update(const ImuSample& s, uint32_t now, EventQueue& q) {
  const float raw[3] = {s.ax, s.ay, s.az};

  // Accelerometers measure specific force (what the floor pushes back with). That vector, mapped
  // to screen axes, is exactly the "gravity" an object inside the device feels: 1 g when resting,
  // 0 in free fall, large and rapidly changing when shaken.
  st_.gravity.x = cfg::SIGN_SCREEN_X * raw[cfg::AXIS_SCREEN_X];
  st_.gravity.y = cfg::SIGN_SCREEN_Y * raw[cfg::AXIS_SCREEN_Y];
  st_.magnitude = sqrtf(s.ax * s.ax + s.ay * s.ay + s.az * s.az);

  // Home orientation: the first reading after calibration, then a slow follower that only moves
  // while the device is calm. Picking the buddy up never pins its eyes to one wall forever.
  if (!restInit_) {
    st_.restGravity = st_.gravity; restInit_ = true;
  } else if (cfg::REST_ADAPT_TAU_S > 0.f && !st_.shaking && !st_.freeFalling &&
             fabsf(st_.magnitude - baseline_) < 0.1f) {
    st_.restGravity.x = phys::smooth(st_.restGravity.x, st_.gravity.x, s.dt, cfg::REST_ADAPT_TAU_S);
    st_.restGravity.y = phys::smooth(st_.restGravity.y, st_.gravity.y, s.dt, cfg::REST_ADAPT_TAU_S);
  }

  const float gyro = sqrtf(s.gx * s.gx + s.gy * s.gy + s.gz * s.gz);
  st_.spinDps = phys::smooth(st_.spinDps, gyro, s.dt, 0.15f);

  trackFreeFall(now, q);
  trackImpact(now, q);
  trackShake(now, s.dt, q);
}

void MotionAnalyzer::trackFreeFall(uint32_t now, EventQueue& q) {
  const float m = st_.magnitude;
  if (!inFF_) {
    if (m < cfg::FF_ENTER_G) {
      if (!lowActive_) { lowActive_ = true; lowSince_ = now; }
      else if (now - lowSince_ >= cfg::FF_MIN_MS) {
        inFF_ = true; ffStart_ = lowSince_; st_.freeFalling = true;
        q.push(makeEvent(EventType::FreeFallStart, 0, 0, now));
      }
    } else {
      lowActive_ = false;
    }
  } else {
    st_.freeFallSeconds = (now - ffStart_) * 0.001f;
    if (m > cfg::FF_EXIT_G) {
      const float dur = st_.freeFallSeconds;
      inFF_ = false; lowActive_ = false; st_.freeFalling = false; st_.freeFallSeconds = 0;
      q.push(makeEvent(EventType::FreeFallEnd, dur, 0, now));
      impactWatch_ = true; impactUntil_ = now + cfg::IMPACT_WINDOW_MS; impactPeak_ = m;
      suppressShakeUntil_ = now + cfg::SHAKE_SUPPRESS_AFTER_FF_MS;  // landing is not a shake
    }
  }
}

void MotionAnalyzer::trackImpact(uint32_t now, EventQueue& q) {
  if (!impactWatch_) return;
  if (st_.magnitude > impactPeak_) impactPeak_ = st_.magnitude;
  if (now >= impactUntil_) {
    impactWatch_ = false;
    if (impactPeak_ > cfg::IMPACT_G) q.push(makeEvent(EventType::Impact, impactPeak_, 0, now));
  }
}

void MotionAnalyzer::trackShake(uint32_t now, float dt, EventQueue& q) {
  // Free fall reads ~0 g, i.e. a 1 g "deviation". It must never count as a shake.
  const bool  eligible = !inFF_ && now >= suppressShakeUntil_;
  const float dev      = eligible ? fabsf(st_.magnitude - baseline_) : 0.f;
  const bool  above    = dev > cfg::SHAKE_THRESH_G;

  st_.shakeLevel = phys::smooth(st_.shakeLevel, dev, dt, dev > st_.shakeLevel ? 0.03f : 0.25f);

  if (above && !wasAbove_) {                       // rising edge = one peak
    if (peaks_ == 0 || now - winStart_ > cfg::SHAKE_WINDOW_MS) {
      peaks_ = 0; winStart_ = now; winEnergy_ = 0;
    }
    peaks_++; winEnergy_ += dev;
    if (st_.shaking) {
      lastPeak_ = now; st_.shakeEnergy += dev;
    } else if (peaks_ >= cfg::SHAKE_PEAKS) {
      st_.shaking = true; shakeStart_ = winStart_; lastPeak_ = now; st_.shakeEnergy = winEnergy_;
      q.push(makeEvent(EventType::ShakeStart, 0, 0, now));
    }
  }
  wasAbove_ = above;

  if (st_.shaking) {
    st_.shakeSeconds = (lastPeak_ - shakeStart_) * 0.001f;
    if (now - lastPeak_ > cfg::SHAKE_END_MS) {     // replaces the old fixed lockout: no double triggers
      q.push(makeEvent(EventType::ShakeEnd, st_.shakeEnergy, st_.shakeSeconds, now));
      st_.shaking = false; st_.shakeEnergy = 0; st_.shakeSeconds = 0; peaks_ = 0;
    }
  }
}
