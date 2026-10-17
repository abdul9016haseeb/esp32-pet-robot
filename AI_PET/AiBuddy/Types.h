// Types.h - plain data shared between modules. No logic, no hardware access.
#pragma once
#include <Arduino.h>

struct Vec2 {
  float x, y;
  Vec2() : x(0.f), y(0.f) {}
  Vec2(float x_, float y_) : x(x_), y(y_) {}
};

// One IMU reading, already in g and degrees/second, gyro bias removed.
struct ImuSample {
  float ax = 0, ay = 0, az = 0;
  float gx = 0, gy = 0, gz = 0;
  float dt = 0.01f;
};

// What the analyzer believes the device is doing right now.
struct MotionState {
  Vec2  gravity;                 // effective gravity in screen axes [g]; (0,0) when weightless
  Vec2  restGravity;             // the 'home' orientation; eyes are centred when gravity == restGravity
  float magnitude       = 1.f;   // |acceleration| [g]
  float spinDps         = 0.f;   // smoothed rotation rate
  float shakeLevel      = 0.f;   // smoothed deviation from 1 g, for continuous effects
  bool  shaking         = false;
  bool  freeFalling     = false;
  float shakeEnergy     = 0.f;   // sum of peak deviations in the current shake
  float shakeSeconds    = 0.f;
  float shakeIntensityG = 0.f;   // MEAN peak deviation of the current shake [g] = how HARD it is (robust to one-off spikes)
  float freeFallSeconds = 0.f;
};

// Discrete things that happened. Add new types at the end; features ignore ones they don't know.
enum class EventType : uint8_t {
  ShakeStart,      // -
  ShakeEnd,        // a = energy, b = duration [s], c = mean peak deviation [g]
  FreeFallStart,   // -
  FreeFallEnd,     // a = fall duration [s]
  Impact,          // a = peak acceleration [g]
  ButtonA,         // reserved for the two buttons
  ButtonB,
  Custom
};

struct Event {
  EventType type;
  float     a;
  float     b;
  float     c;
  uint32_t  timeMs;
};

inline Event makeEvent(EventType t, float a, float b, uint32_t now, float c = 0.f) {
  Event e; e.type = t; e.a = a; e.b = b; e.c = c; e.timeMs = now; return e;
}

// Tiny fixed ring buffer. Drops the newest event if full (never blocks, never allocates).
class EventQueue {
 public:
  bool push(const Event& e) {
    uint8_t n = (head_ + 1) % CAP;
    if (n == tail_) return false;
    buf_[head_] = e; head_ = n; return true;
  }
  bool pop(Event& e) {
    if (head_ == tail_) return false;
    e = buf_[tail_]; tail_ = (tail_ + 1) % CAP; return true;
  }
 private:
  static const uint8_t CAP = 16;
  Event   buf_[CAP];
  uint8_t head_ = 0, tail_ = 0;
};

// What the "mind" (Behavior or any Feature) WANTS the eyes to look like.
// EyePhysics smooths these and turns them into motion. One-shot fields are consumed each tick.
struct EyeIntent {
  float anger     = 0.f;   // 0..1 slanted lids
  float dizzy     = 0.f;   // 0..1 spirals / orbiting
  float widen     = 1.f;   // size multiplier (0.6..1.4)
  float floaty    = 0.f;   // 0..1 weightlessness (soft springs + drift)
  float tremble   = 0.f;   // 0..1 shiver
  bool  autoBlink = true;
  float squash    = 0.f;   // one-shot: instant vertical squash 0..0.65
  Vec2  kick;              // one-shot: velocity impulse [px/s]
};

// Final geometry handed to the renderer.
struct EyeFrame {
  Vec2  pos[2];            // offset of each eye from its resting centre [px]
  float w = 32, h = 34, radius = 9;
  float scale = 1;         // overall size multiplier (used for spirals)
  float anger = 0, dizzy = 0, dizzyPhase = 0;
};