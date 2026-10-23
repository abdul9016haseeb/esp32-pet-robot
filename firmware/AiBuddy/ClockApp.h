// ClockApp.h - the CLOCK menu entry: Clock, Timer and Stopwatch behind one
// top tab bar, exactly the "sections in the UI" layout that was chosen.
//
//   LEFT / RIGHT : switch page (wraps around)
//   CLOCK page   : CENTER enters time edit (HH -> MM -> SS -> done),
//                  UP / DOWN adjust the underlined field (hold = repeat)
//   TIMER page   : UP / DOWN set the duration (hold = repeat), CENTER start /
//                  pause / resume, DOWN (paused) reset, DOWN (finished) reset
//   STOPWATCH    : CENTER start / stop, UP lap (while running), DOWN reset (stopped)
//
// No on-screen hints (agreed design). State is shown by marks only:
// flashing colon = running, underline = editable field, PAUSED / TIME'S UP tags.
// All drawing is non-blocking and uses the 6x8 font (see Config.h for layout).
#pragma once
#include <Adafruit_GFX.h>
#include "App.h"
#include "Types.h"

class ClockService;

class ClockApp : public App {
 public:
  explicit ClockApp(ClockService& svc) : svc_(svc) {}
  const char* name() const override { return "clock"; }
  void onEnter(Context&) override;                        // reset edit / repeat state
  void onExit(Context&)  override;
  void update(Context&)  override;                        // drives edit auto-repeat
  void onButton(Context&, Button, bool pressed) override;
  void draw(Context&, Adafruit_GFX&) override;

 private:
  enum class Page : uint8_t { Clock = 0, Timer = 1, Stopwatch = 2 };

  void drawTabs(Adafruit_GFX& g) const;
  void drawClockPage(Context& ctx, Adafruit_GFX& g) const;
  void drawTimerPage(Context& ctx, Adafruit_GFX& g) const;
  void drawStopwatchPage(Context& ctx, Adafruit_GFX& g) const;

  void clockButton(Context& ctx, Button b, bool pressed);
  void adjustEditedField(Button b);                       // UP/DOWN on the underlined field
  void repeatUpdate(Context& ctx);

  ClockService& svc_;
  Page     page_      = Page::Clock;
  uint8_t  editField_ = 0;        // clock page: 0 = none, 1 = HH, 2 = MM, 3 = SS

  // held-button auto-repeat for time edit (same pattern as ClockService)
  Button   repBtn_    = Button::Count;
  bool     repHeld_   = false;
  uint32_t repNextMs_ = 0;
};
