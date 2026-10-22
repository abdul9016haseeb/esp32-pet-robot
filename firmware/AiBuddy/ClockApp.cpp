#include "ClockApp.h"
#include "ClockService.h"
#include "Config.h"
#include "Ui.h"
#include <string.h>
#include <stdio.h>

// ---- big text helpers (built-in font at size 2: 12x16 px per char) ----
static void bigText(Adafruit_GFX& g, int x, int y, const char* s, uint16_t c) {
  g.setTextSize(2);
  g.setTextColor(c);
  g.setTextWrap(false);
  g.setCursor(x, y);
  g.print(s);
  g.setTextSize(1);
}

static void bigCentered(Adafruit_GFX& g, int y, const char* s, uint16_t c) {
  bigText(g, (cfg::SCREEN_W - (int)strlen(s) * ui::CHAR_W * 2) / 2, y, s, c);
}

// Flash helper: true for half of each blink period (running colon, TIME'S UP).
static bool blinkPhase(uint32_t nowMs) {
  return ((nowMs / cfg::CLOCK_BLINK_HALF_MS) & 1u) == 0;
}

// ------------------------------------------------------------------ lifecycle

void ClockApp::onEnter(Context&) {
  editField_ = 0;
  repHeld_   = false;
}

void ClockApp::onExit(Context&) {
  editField_ = 0;
  repHeld_   = false;
}

void ClockApp::update(Context& ctx) {
  repeatUpdate(ctx);
}

// ------------------------------------------------------------------ input

void ClockApp::onButton(Context& ctx, Button b, bool pressed) {
  // LEFT / RIGHT always switch pages (wrap), from every page.
  if (pressed && b == Button::Left)  { page_ = (Page)(((uint8_t)page_ + 2) % 3); editField_ = 0; repHeld_ = false; return; }
  if (pressed && b == Button::Right) { page_ = (Page)(((uint8_t)page_ + 1) % 3); editField_ = 0; repHeld_ = false; return; }

  switch (page_) {
    case Page::Clock:     clockButton(ctx, b, pressed);      break;
    case Page::Timer:     svc_.timerButton(ctx, b, pressed); break;
    case Page::Stopwatch: svc_.swButton(ctx, b, pressed);    break;
  }
}

void ClockApp::clockButton(Context& ctx, Button b, bool pressed) {
  if (b == Button::Center && pressed) {
    repHeld_ = false;
    if (++editField_ > 3) editField_ = 0;      // none -> HH -> MM -> SS -> none
    return;
  }
  if (b != Button::Up && b != Button::Down) return;
  if (editField_ == 0) return;                 // not editing: UP/DOWN do nothing here
  if (pressed) {
    adjustEditedField(b);
    repBtn_ = b; repHeld_ = true;
    repNextMs_ = ctx.nowMs + cfg::BTN_REPEAT_DELAY_MS;
  } else if (repHeld_ && b == repBtn_) {
    repHeld_ = false;
  }
}

void ClockApp::adjustEditedField(Button b) {
  const int sign = (b == Button::Up) ? 1 : -1;
  const int step = (editField_ == 1) ? 3600 : (editField_ == 2) ? 60 : 1;
  svc_.adjustTime(sign * step);
}

void ClockApp::repeatUpdate(Context& ctx) {
  if (!repHeld_ || editField_ == 0) return;
  if ((int32_t)(ctx.nowMs - repNextMs_) < 0) return;
  adjustEditedField(repBtn_);
  repNextMs_ += cfg::BTN_REPEAT_PERIOD_MS;
}

// ------------------------------------------------------------------ drawing

void ClockApp::draw(Context& ctx, Adafruit_GFX& g) {
  drawTabs(g);
  switch (page_) {
    case Page::Clock:     drawClockPage(ctx, g);     break;
    case Page::Timer:     drawTimerPage(ctx, g);     break;
    case Page::Stopwatch: drawStopwatchPage(ctx, g); break;
  }
}

void ClockApp::drawTabs(Adafruit_GFX& g) const {
  static const char* const LABELS[3] = {"CLOCK", "TIMER", "STOPW"};
  const int slotW = cfg::SCREEN_W / 3;
  for (uint8_t i = 0; i < 3; i++) {
    const int x     = i * slotW;
    const bool act  = (uint8_t)page_ == i;
    const int tw    = ui::textWidth(LABELS[i]);
    if (act) g.fillRect(x + 1, 1, slotW - 2, cfg::CLOCK_TAB_H - 2, cfg::C_ON);
    ui::text(g, x + (slotW - tw) / 2, 2, LABELS[i], act ? cfg::C_OFF : cfg::C_ON, act);
  }
  g.drawFastHLine(0, cfg::CLOCK_TAB_H, cfg::SCREEN_W, cfg::C_ON);   // separator under the bar
}

void ClockApp::drawClockPage(Context& ctx, Adafruit_GFX& g) const {
  (void)ctx;
  const int tod = svc_.timeOfDaySec();
  char buf[9];
  snprintf(buf, sizeof buf, "%02d:%02d:%02d", tod / 3600, (tod / 60) % 60, tod % 60);
  bigCentered(g, 22, buf, cfg::C_ON);
  if (editField_ != 0) {
    // underline the active field; at size 2 a field is 2 chars (24 px) + a colon,
    // so HH starts at x0, MM at x0 + 36, SS at x0 + 72
    const int x0 = (cfg::SCREEN_W - 8 * ui::CHAR_W * 2) / 2;
    g.drawFastHLine(x0 + (editField_ - 1) * 3 * ui::CHAR_W * 2, 41, 2 * ui::CHAR_W * 2, cfg::C_ON);
  }
}

void ClockApp::drawTimerPage(Context& ctx, Adafruit_GFX& g) const {
  const ClockService::TimerState st = svc_.timerState();
  const uint32_t ms = svc_.timerLeftMs();
  char buf[6];
  snprintf(buf, sizeof buf, "%02u:%02u", (unsigned)(ms / 60000UL), (unsigned)(ms / 1000UL) % 60UL);
  if (st == ClockService::TimerState::Running && !blinkPhase(ctx.nowMs)) buf[2] = ' ';  // flashing colon
  bigCentered(g, 22, buf, cfg::C_ON);
  if (st == ClockService::TimerState::Idle) {
    const int w = 5 * ui::CHAR_W * 2;
    g.drawFastHLine((cfg::SCREEN_W - w) / 2, 41, w, cfg::C_ON);      // underline: adjustable
  } else if (st == ClockService::TimerState::Paused) {
    ui::textCentered(g, 48, "PAUSED", cfg::C_ON, true);
  } else if (st == ClockService::TimerState::Finished && blinkPhase(ctx.nowMs)) {
    ui::textCentered(g, 48, "TIME'S UP", cfg::C_ON, true);
  }
}

void ClockApp::drawStopwatchPage(Context& ctx, Adafruit_GFX& g) const {
  const uint32_t ms = svc_.swElapsedMs();
  char buf[12];
  snprintf(buf, sizeof buf, "%02u:%02u.%02u",
           (unsigned)(ms / 60000UL), (unsigned)(ms / 1000UL) % 60UL, (unsigned)(ms / 10UL) % 100UL);
  if (svc_.swRunning() && !blinkPhase(ctx.nowMs)) buf[5] = ' ';       // blink the dot while running
  bigCentered(g, 18, buf, cfg::C_ON);

  // last three laps, newest first: "L3 01:23.45"
  const uint8_t n = svc_.swLapCount();
  for (uint8_t i = 0; i < 3 && i < n; i++) {
    const uint32_t lap = svc_.swLapMs(i);
    char line[16];
    snprintf(line, sizeof line, "L%u %02u:%02u.%02u", (unsigned)(n - i),
             (unsigned)(lap / 60000UL), (unsigned)(lap / 1000UL) % 60UL, (unsigned)(lap / 10UL) % 100UL);
    ui::text(g, 20, 42 + i * ui::CHAR_H, line, cfg::C_ON);
  }
}
