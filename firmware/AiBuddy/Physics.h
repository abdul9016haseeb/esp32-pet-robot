// Physics.h - small, allocation-free spring/damper toolkit.
#pragma once
#include <math.h>
#include "Types.h"

namespace phys {

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Frame-rate independent exponential smoothing. tau = time constant in seconds.
inline float smooth(float cur, float target, float dt, float tau) {
  return cur + (target - cur) * (1.f - expf(-dt / tau));
}

constexpr float MAX_SUBSTEP = 0.008f;  // keeps semi-implicit Euler stable for stiff springs

// Scalar spring: pos is pulled to target.
struct Spring1D {
  float pos = 1.f, vel = 0.f, target = 1.f, k = 260.f, c = 13.f;
  void step(float dt) {
    int n = (int)ceilf(dt / MAX_SUBSTEP); if (n < 1) n = 1;
    const float h = dt / n;
    for (int i = 0; i < n; i++) {
      vel += (-k * (pos - target) - c * vel) * h;
      pos += vel * h;
    }
  }
};

// 2D mass on a spring with an external acceleration, clamped to a box with bounce.
struct Body2D {
  Vec2  pos, vel;
  float k = 480.f, c = 14.f, restitution = 0.45f;
  Vec2  limit = Vec2(11.f, 10.f);

  static void wall(float& p, float& v, float lim, float e) {
    if (p > lim)       { p = lim;  if (v > 0) v = -v * e; }
    else if (p < -lim) { p = -lim; if (v < 0) v = -v * e; }
  }

  void step(float dt, const Vec2& ext, float kMul, float cMul) {
    int n = (int)ceilf(dt / MAX_SUBSTEP); if (n < 1) n = 1;
    const float h = dt / n;
    for (int i = 0; i < n; i++) {
      vel.x += (-k * kMul * pos.x - c * cMul * vel.x + ext.x) * h;
      vel.y += (-k * kMul * pos.y - c * cMul * vel.y + ext.y) * h;
      pos.x += vel.x * h;
      pos.y += vel.y * h;
      wall(pos.x, vel.x, limit.x, restitution);
      wall(pos.y, vel.y, limit.y, restitution);
    }
  }
};

}  // namespace phys
