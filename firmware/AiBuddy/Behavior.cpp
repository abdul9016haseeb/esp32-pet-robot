#include "Behavior.h"
#include "Config.h"
#include "Physics.h"

const char* Behavior::modeName() const {
  switch (mode_) {
    case Mode::Idle:     return "idle";
    case Mode::Shaking:  return "shaking";
    case Mode::Dizzy:    return "dizzy";
    case Mode::Angry:    return "angry";
    case Mode::FreeFall: return "freefall";
    case Mode::Landing:  return "landing";
  }
  return "?";
}

void Behavior::begin(Context& ctx) {
  enter(Mode::Idle, ctx);
  nextGlanceMs_ = ctx.nowMs + random((long)cfg::GLANCE_MIN_MS, (long)cfg::GLANCE_MAX_MS);
}

// Each mode is nothing more than a preset of eye intent.
void Behavior::enter(Mode m, Context& ctx) {
  mode_ = m;
  modeSince_ = ctx.nowMs;
  EyeIntent& in = *ctx.intent;
  in.anger = 0; in.dizzy = 0; in.widen = 1.f; in.floaty = 0; in.tremble = 0; in.autoBlink = true;
  switch (m) {
    case Mode::Idle:     break;
    case Mode::Shaking:  in.widen = 1.12f; in.tremble = 0.4f; in.autoBlink = false; break;
    case Mode::Dizzy:    in.dizzy = 1.f; in.widen = 0.95f; in.autoBlink = false; break;
    case Mode::Angry:    in.anger = 1.f; in.widen = 0.92f; in.tremble = 0.5f; in.autoBlink = false; break;
    case Mode::FreeFall: in.widen = 1.3f; in.floaty = 1.f; in.autoBlink = false; break;
    case Mode::Landing:  in.widen = 1.1f; in.autoBlink = false; break;
  }
  BUDDY_LOG("[behavior] -> %s\n", modeName());
}

void Behavior::onEvent(Context& ctx, const Event& e) {
  switch (e.type) {
    case EventType::ShakeStart:
      spinTimeS_ = 0;
      enter(Mode::Shaking, ctx);
      break;
    case EventType::ShakeEnd:
      if (mode_ == Mode::Shaking) onShakeEnd(ctx, e);
      break;
    case EventType::FreeFallStart:
      enter(Mode::FreeFall, ctx);
      break;
    case EventType::FreeFallEnd:
      fallSeconds_ = e.a;
      if (mode_ == Mode::FreeFall) enter(e.a >= cfg::MIN_FALL_S ? Mode::Landing : Mode::Idle, ctx);
      break;
    case EventType::Impact:
      if (mode_ == Mode::Landing) onImpact(ctx, e.a);
      break;
    default:
      break;  // buttons etc.: not this feature's business
  }
}

// Decision rule for a finished shake, based on how HARD it was (strongest peak deviation, in g):
//   gentle shake (intensity below HARD_SHAKE_INTENSITY_G) -> Angry  "stop poking me"
//   hard shake   (intensity at/above it)                  -> Dizzy  spiral eyes
// Intensity = MEAN peak deviation of the shake [g], so one stray spike cannot make a gentle shake "hard".
// To add a third tier (e.g. a scared reaction for extreme shakes), add one more branch here.
Behavior::Reaction Behavior::chooseShakeReaction(float energy, float seconds, float intensityG) const {
  if (intensityG >= cfg::HARD_SHAKE_INTENSITY_G) {
    const float hold = cfg::DIZZY_BASE_S + energy * cfg::DIZZY_PER_ENERGY;
    return { Mode::Dizzy, phys::clampf(hold, cfg::DIZZY_BASE_S, cfg::DIZZY_MAX_S) };
  }
  const float hold = cfg::ANGRY_BASE_S + seconds * cfg::ANGRY_PER_SECOND;
  return { Mode::Angry, phys::clampf(hold, cfg::ANGRY_BASE_S, cfg::ANGRY_MAX_S) };
}

void Behavior::onShakeEnd(Context& ctx, const Event& e) {
  const float energy = e.a, seconds = e.b, intensityG = e.c;
  const Reaction r = chooseShakeReaction(energy, seconds, intensityG);
  holdS_ = r.holdS;
  BUDDY_LOG("[behavior] shake intensity %.2f g (hard >= %.2f g)\n", intensityG, cfg::HARD_SHAKE_INTENSITY_G);
  enter(r.mode, ctx);
}

// Landing: squash proportional to impact, then dizzy only after a long fall or a hard hit.
void Behavior::onImpact(Context& ctx, float peakG) {
  ctx.intent->squash = phys::clampf((peakG - 1.f) * 0.18f, 0.15f, 0.6f);
  ctx.intent->kick   = Vec2(0.f, 120.f);
  if (fallSeconds_ > cfg::DIZZY_FALL_S || peakG > cfg::DIZZY_IMPACT_G) {
    holdS_ = 1.5f + fallSeconds_;
    enter(Mode::Dizzy, ctx);
  } else {
    enter(Mode::Idle, ctx);
  }
}

void Behavior::update(Context& ctx) {
  const float inMode = (ctx.nowMs - modeSince_) * 0.001f;

  switch (mode_) {
    case Mode::Idle:
      updateIdle(ctx);
      break;
    case Mode::Dizzy:
    case Mode::Angry:
      if (inMode >= holdS_) enter(Mode::Idle, ctx);
      break;
    case Mode::Landing:
      if (inMode > cfg::LANDING_WAIT_S) enter(Mode::Idle, ctx);   // caught softly, no impact
      break;
    case Mode::Shaking:   // watchdogs in case an end event was dropped
      if (!ctx.motion->shaking && inMode > 0.5f) enter(Mode::Idle, ctx);
      break;
    case Mode::FreeFall:
      if (!ctx.motion->freeFalling && inMode > 0.3f) enter(Mode::Idle, ctx);
      break;
  }
}

void Behavior::updateIdle(Context& ctx) {
  // Occasional look-around: an impulse, the spring physics does the rest.
  if (ctx.nowMs >= nextGlanceMs_) {
    ctx.intent->kick = Vec2(random(-100, 101) / 100.f * cfg::GLANCE_SPEED,
                            random(-60, 61) / 100.f * cfg::GLANCE_SPEED * 0.6f);
    nextGlanceMs_ = ctx.nowMs + random((long)cfg::GLANCE_MIN_MS, (long)cfg::GLANCE_MAX_MS);
  }
  // Spinning (gyro) makes it dizzy, like a person on a chair.
  if (ctx.motion->spinDps > cfg::SPIN_DIZZY_DPS) spinTimeS_ += ctx.dt;
  else spinTimeS_ = fmaxf(0.f, spinTimeS_ - ctx.dt);
  if (spinTimeS_ > cfg::SPIN_DIZZY_AFTER_S) {
    spinTimeS_ = 0; holdS_ = 2.f;
    enter(Mode::Dizzy, ctx);
  }
}