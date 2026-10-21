#include "ClockService.h"
#include "Config.h"

void ClockService::begin(Context& ctx) {
  anchorMs_ = ctx.nowMs;
  tleftMs_  = tsetMs_;                      // stay Idle, just re-arm the countdown
}

void ClockService::update(Context& ctx) {
  // ---- countdown timer: burn down real elapsed time, fire once at zero ----
  if (tstate_ == TimerState::Running) {
    const uint32_t step = (uint32_t)(ctx.dt * 1000.0f);   // dt is clamped by the scheduler
    if (step >= tleftMs_) {
      tleftMs_ = 0;
      tstate_  = TimerState::Finished;
      if (ctx.events) ctx.events->push(makeEvent(EventType::TimerFinished, 0, 0, ctx.nowMs));
    } else {
      tleftMs_ -= step;
    }
  }

  // ---- stopwatch ----
  if (swRun_) swMs_ += (uint32_t)(ctx.dt * 1000.0f);

  repeatUpdate(ctx);
}

// ---------------------------------------------------------------- wall clock

int ClockService::timeOfDaySec() const {
  // unsigned subtraction keeps this correct across a millis() wrap (~49 days)
  return (int)((baseSec_ + (millis() - anchorMs_) / 1000UL) % 86400UL);
}

void ClockService::adjustTime(int deltaSec) {
  int s = baseSec_ + deltaSec;
  s %= 86400;
  if (s < 0) s += 86400;
  baseSec_ = s;
}

// ---------------------------------------------------------------- timer

void ClockService::timerAdjust(int deltaSec) {
  if (tstate_ != TimerState::Idle) return;
  int32_t s = (int32_t)(tsetMs_ / 1000UL) + deltaSec;
  if (s < 0)                            s = 0;
  if (s > (int32_t)cfg::TIMER_MAX_SEC)  s = cfg::TIMER_MAX_SEC;
  tsetMs_  = (uint32_t)s * 1000UL;
  tleftMs_ = tsetMs_;
}

void ClockService::timerStart() {
  if (tstate_ == TimerState::Idle || tstate_ == TimerState::Finished) {
    tleftMs_ = tsetMs_;
    tstate_  = TimerState::Running;
  }
}

void ClockService::timerPause()  { if (tstate_ == TimerState::Running) tstate_ = TimerState::Paused;   }
void ClockService::timerResume() { if (tstate_ == TimerState::Paused)  tstate_ = TimerState::Running;  }
void ClockService::timerReset()  { tleftMs_ = tsetMs_; tstate_ = TimerState::Idle; }

void ClockService::timerButton(Context& ctx, Button b, bool pressed) {
  if (b == Button::Up || b == Button::Down) {
    if (pressed) {
      if (tstate_ == TimerState::Idle) {              // adjust the set duration (with hold-repeat)
        timerAdjust(b == Button::Up ? cfg::TIMER_ADJUST_STEP_SEC : -cfg::TIMER_ADJUST_STEP_SEC);
        repBtn_ = b; repHeld_ = true;
        repNextMs_ = ctx.nowMs + cfg::BTN_REPEAT_DELAY_MS;
      } else if (tstate_ == TimerState::Paused || tstate_ == TimerState::Finished) {
        if (b == Button::Down) timerReset();          // DOWN = back to the set duration
      }
    } else if (repHeld_ && b == repBtn_) {
      repHeld_ = false;                               // released: stop repeating
    }
    return;
  }
  if (b == Button::Center && pressed) {
    repHeld_ = false;
    switch (tstate_) {
      case TimerState::Idle:      timerStart();  break;
      case TimerState::Running:   timerPause();  break;
      case TimerState::Paused:    timerResume(); break;
      case TimerState::Finished:  timerStart();  break;   // one more CENTER restarts it
    }
  }
}

// ------------------------------------------------------------- stopwatch

void ClockService::swButton(Context& ctx, Button b, bool pressed) {
  (void)ctx;
  if (!pressed) return;
  if      (b == Button::Center) swRun_ = !swRun_;       // start / stop
  else if (b == Button::Up)     swLap();
  else if (b == Button::Down)   swReset();
}

uint32_t ClockService::swLapMs(uint8_t i) const {
  return (i < nlaps_) ? laps_[i] : 0;
}

void ClockService::swLap() {
  if (!swRun_) return;
  const uint32_t dur = swMs_ - lastSplitMs_;
  lastSplitMs_ = swMs_;
  for (uint8_t i = 7; i > 0; i--) laps_[i] = laps_[i - 1];   // shift the ring, drop the oldest
  laps_[0] = dur;
  if (nlaps_ < 8) nlaps_++;
}

void ClockService::swReset() {
  if (swRun_) return;                                   // only when stopped
  swMs_ = 0; lastSplitMs_ = 0; nlaps_ = 0;
}

// ------------------------------------------------------- held-button repeat

void ClockService::repeatUpdate(Context& ctx) {
  if (!repHeld_) return;
  if ((int32_t)(ctx.nowMs - repNextMs_) < 0) return;
  if (tstate_ == TimerState::Idle) {
    timerAdjust(repBtn_ == Button::Up ? cfg::TIMER_ADJUST_STEP_SEC : -cfg::TIMER_ADJUST_STEP_SEC);
    repNextMs_ += cfg::BTN_REPEAT_PERIOD_MS;            // fixed cadence; step is tunable in Config.h
  } else {
    repHeld_ = false;                                   // state changed under our feet
  }
}
