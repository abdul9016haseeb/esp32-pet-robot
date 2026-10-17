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
      if (mode_ == Mode::Shaking) onShakeEnd(ctx, e.a, e.b);
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

// After a shake: dizzy if it was brief, angry if it was long or the buddy was already annoyed.
void Behavior::onShakeEnd(Context& ctx, float energy, float seconds) {
  annoyance_ += energy;
  if (annoyance_ > cfg::ANGRY_ENERGY || seconds > cfg::ANGRY_SECONDS) {
    holdS_ = phys::clampf(cfg::ANGRY_BASE_S + seconds * 0.5f, cfg::ANGRY_BASE_S, cfg::ANGRY_MAX_S);
    annoyance_ = 0;
    enter(Mode::Angry, ctx);
  } else {
    holdS_ = phys::clampf(cfg::DIZZY_BASE_S + energy * cfg::DIZZY_PER_ENERGY, cfg::DIZZY_BASE_S, cfg::DIZZY_MAX_S);
    enter(Mode::Dizzy, ctx);
  }
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
  annoyance_ *= expf(-ctx.dt / cfg::ANNOYANCE_TAU_S);
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
