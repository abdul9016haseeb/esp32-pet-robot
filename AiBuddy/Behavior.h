// Behavior.h - the "personality": maps motion events to emotional states and eye intent.
// It is just another Feature, so it can be replaced or joined by others.
#pragma once
#include "Feature.h"

class Behavior : public Feature {
 public:
  const char* name() const override { return "behavior"; }
  void begin(Context& ctx) override;
  void update(Context& ctx) override;
  void onEvent(Context& ctx, const Event& e) override;
  const char* modeName() const;

 private:
  enum class Mode : uint8_t { Idle, Shaking, Dizzy, Angry, FreeFall, Landing };

  void enter(Mode m, Context& ctx);
  void updateIdle(Context& ctx);
  void onShakeEnd(Context& ctx, float energy, float seconds);
  void onImpact(Context& ctx, float peakG);

  Mode     mode_ = Mode::Idle;
  uint32_t modeSince_ = 0, nextGlanceMs_ = 0;
  float    holdS_ = 0;          // how long Dizzy/Angry lasts
  float    annoyance_ = 0;      // leaky integrator of recent shaking
  float    fallSeconds_ = 0;
  float    spinTimeS_ = 0;
};
