// Feature.h - the extension point. To add clock, timer, buttons, WiFi, music:
//   1. derive from Feature, 2. features.add(&yourFeature) in setup().
// A feature can read motion, steer the eyes through ctx.intent, react to Events,
// and draw overlays after the eyes (draw()).
// Overlay features should check ctx.screen: the eyes (and any overlay meant for them) are only
// visible while ctx.screen == Screen::Eyes.
#pragma once
#include <Adafruit_GFX.h>
#include "Types.h"

struct Context {
  uint32_t           nowMs  = 0;
  float              dt     = 0;
  const MotionState* motion = nullptr;
  EyeIntent*         intent = nullptr;
  const EyeFrame*    frame  = nullptr;
  EventQueue*        events = nullptr;             // features may push their own events
  Screen             screen = Screen::Eyes;        // what is on the display right now
};

class Feature {
 public:
  virtual ~Feature() {}
  virtual const char* name() const = 0;
  virtual void begin(Context&) {}
  virtual void update(Context&) {}                 // every simulation tick
  virtual void onEvent(Context&, const Event&) {}  // every event, in order
  virtual void draw(Context&, Adafruit_GFX&) {}    // after the eyes, every render
};

class FeatureRegistry {
 public:
  bool add(Feature* f) { if (n_ >= MAX) return false; items_[n_++] = f; return true; }
  void beginAll(Context& c)                  { for (uint8_t i = 0; i < n_; i++) items_[i]->begin(c); }
  void updateAll(Context& c)                 { for (uint8_t i = 0; i < n_; i++) items_[i]->update(c); }
  void dispatch(Context& c, const Event& e)  { for (uint8_t i = 0; i < n_; i++) items_[i]->onEvent(c, e); }
  void drawAll(Context& c, Adafruit_GFX& g)  { for (uint8_t i = 0; i < n_; i++) items_[i]->draw(c, g); }
 private:
  static const uint8_t MAX = 8;
  Feature* items_[MAX];
  uint8_t  n_ = 0;
};
