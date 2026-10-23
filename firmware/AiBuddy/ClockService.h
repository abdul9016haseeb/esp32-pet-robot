// ClockService.h - one Feature owns all timekeeping: wall clock, countdown
// timer, stopwatch. It keeps ticking in EVERY screen state, so a timer set in
// the app still fires while the user is back at the eyes (the Shell then takes
// over the screen via the TimerFinished event).
// Pure state + math: no drawing, no hardware, no button decisions of its own -
// ClockApp forwards buttons to it. All timing uses ctx.dt / ctx.nowMs, so it
// stays correct even if the loop runs slower than the 10 ms plan.
#pragma once
#include "Feature.h"
#include "Types.h"

class ClockService : public Feature {
 public:
  const char* name() const override { return "clocksvc"; }
  void begin(Context& ctx) override;
  void update(Context& ctx) override;

  // ---- Wall clock: seconds since midnight. There is no RTC (see guide notes),
  // so the time is millis()-based and is lost on reset; editing shifts it. ----
  int  timeOfDaySec() const;
  void adjustTime(int deltaSec);          // edit mode: shift the wall clock

  // ---- Countdown timer. Idle = set duration shown, adjustable. ----
  enum class TimerState : uint8_t { Idle, Running, Paused, Finished };
  TimerState timerState()    const { return tstate_; }
  uint32_t   timerSetMs()    const { return tsetMs_; }   // last set duration
  uint32_t   timerLeftMs()   const { return tleftMs_; }
  void timerAdjust(int deltaSec);         // Idle only: change the set duration
  void timerStart();                      // Idle/Finished -> Running from the set duration
  void timerPause();                      // Running -> Paused
  void timerResume();                     // Paused -> Running (continues from where it stopped)
  void timerReset();                      // Paused/Finished -> Idle at the set duration
  void timerButton(Context& ctx, Button b, bool pressed);   // UP/DOWN adjust (Idle), CENTER start/pause/resume,
                                                            // DOWN reset (Paused/Finished)

  // ---- Stopwatch ----
  bool     swRunning()   const { return swRun_; }
  uint32_t swElapsedMs() const { return swMs_; }
  uint8_t  swLapCount()  const { return nlaps_; }
  uint32_t swLapMs(uint8_t i) const;      // duration of lap i; i = 0 is the most recent
  void swButton(Context& ctx, Button b, bool pressed);      // CENTER start/stop, UP lap, DOWN reset (stopped)

 private:
  void repeatUpdate(Context& ctx);        // auto-repeat for a held UP/DOWN
  void swLap();                           // record a lap (only while running)
  void swReset();                         // clear everything (only while stopped)

  // wall clock
  uint32_t anchorMs_ = 0;    // ctx.nowMs when begin() ran
  int      baseSec_  = 0;    // time of day at the anchor

  // countdown timer
  TimerState tstate_ = TimerState::Idle;
  uint32_t tsetMs_   = 5UL * 60UL * 1000UL;    // default set duration: 5:00
  uint32_t tleftMs_  = tsetMs_;

  // stopwatch
  bool     swRun_ = false;
  uint32_t swMs_  = 0;
  uint32_t lastSplitMs_ = 0;                 // elapsed time when the last lap was taken
  uint32_t laps_[8];                         // ring of recent lap durations [ms], laps_[0] = newest
  uint8_t  nlaps_ = 0;

  // held-button auto-repeat (used by timer adjust and clock edit)
  Button   repBtn_    = Button::Count;
  bool     repHeld_   = false;
  uint32_t repNextMs_ = 0;
};
