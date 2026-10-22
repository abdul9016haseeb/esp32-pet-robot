#include "Shell.h"
#include "Config.h"
#include "Ui.h"

static const char* screenName(Screen s) {
  switch (s) {
    case Screen::Eyes: return "eyes";
    case Screen::Menu: return "menu";
    case Screen::App:  return "app";
    default:           return "alert";
  }
}

void Shell::show(Screen s, Context& ctx) {
  screen_ = s;
  ctx.screen = s;                      // later events in the same tick see the new screen
  BUDDY_LOG("[shell] -> %s\n", screenName(s));
}

bool Shell::onEvent(Context& ctx, const Event& e) {
  if (e.type == EventType::TimerFinished) { startAlert(ctx); return true; }   // takeover from ANY screen

  if (e.type != EventType::ButtonPressed && e.type != EventType::ButtonReleased) return false;
  const Button b = buttonOf(e);
  const bool pressed = e.type == EventType::ButtonPressed;

  // While the alert is up it owns every button: lockout first (so a button
  // being mashed when the timer fires cannot dismiss it instantly), then
  // CENTER or EXIT dismisses, everything else is swallowed.
  if (alert_) {
    if ((uint32_t)(ctx.nowMs - alertStartMs_) < cfg::ALERT_LOCKOUT_MS) return true;
    if (b == Button::Exit && !pressed)       { dismissAlert(ctx); return true; }
    if (b == Button::Center && pressed)      { dismissAlert(ctx); return true; }
    return true;
  }

  // EXIT belongs to the framework and acts on RELEASE (so a game that reacts to the press
  // has nothing to do with it). Bounce shorter than BTN_DEBOUNCE_MS never reaches this point.
  if (b == Button::Exit) {
    if (!pressed) {
      if (screen_ == Screen::App)       closeApp(ctx);
      else if (screen_ == Screen::Menu) show(Screen::Eyes, ctx);
    }
    return true;
  }

  switch (screen_) {
    case Screen::Eyes:
      if (pressed && b == Button::Center) { show(Screen::Menu, ctx); return true; }
      return false;                                  // other buttons stay available to Features
    case Screen::Menu:
      if (pressed) onMenuButton(ctx, b);
      return true;
    case Screen::App:
      routeToApp(ctx, b, pressed);
      return true;
  }
  return false;
}

void Shell::onMenuButton(Context& ctx, Button b) {
  switch (b) {
    case Button::Up:   menu_.move(-1); break;
    case Button::Down: menu_.move(+1); break;
    case Button::Center: {
      const MenuEntry* sel = menu_.selected();
      if (!sel) break;
      if (sel->app) launch(sel->app, ctx);
      else          show(Screen::Eyes, ctx);          // the EXIT row
      break;
    }
    default: break;                                  // LEFT / RIGHT unused in the menu
  }
}

void Shell::launch(App* app, Context& ctx) {
  active_ = app;
  held_ = 0;                                         // the CENTER release that follows is not the app's
  show(Screen::App, ctx);
  active_->onEnter(ctx);
}

void Shell::closeApp(Context& ctx) {
  if (active_) active_->onExit(ctx);
  active_ = nullptr;
  held_ = 0;
  show(Screen::Menu, ctx);
}

// An app only receives a release for a button it saw pressed, so the CENTER press that
// launched it cannot leak into the app as a stray release.
void Shell::routeToApp(Context& ctx, Button b, bool pressed) {
  if (!active_) return;
  const uint8_t bit = (uint8_t)(1u << (uint8_t)b);
  if (pressed) held_ |= bit;
  else if (held_ & bit) held_ &= (uint8_t)~bit;
  else return;
  active_->onButton(ctx, b, pressed);
}

void Shell::update(Context& ctx) {
  if (alert_) {
    if (buzzer_) buzzer_->update(ctx.nowMs);   // keeps beeping; gives up on its own after the timeout
    return;                                    // a running app was already closed by startAlert()
  }
  if (screen_ == Screen::App && active_) active_->update(ctx);
}

void Shell::draw(Context& ctx, Adafruit_GFX& g) {
  if (alert_)                                  { drawAlert(ctx, g); return; }
  if (screen_ == Screen::Menu)                 menu_.draw(g);
  else if (screen_ == Screen::App && active_)  active_->draw(ctx, g);
}

// The timer-finished takeover: full-screen flashing page. Works with or without
// a buzzer; CENTER or EXIT dismisses and returns to the eyes (agreed rule).
void Shell::startAlert(Context& ctx) {
  if (alert_) return;                         // already alerting: the screen stays, do not restart
  alert_ = true;
  alertStartMs_ = ctx.nowMs;
  if (screen_ == Screen::App) closeApp(ctx);  // the running app gets onExit first
  show(Screen::Alert, ctx);
  if (buzzer_) buzzer_->start(ctx.nowMs);
  BUDDY_LOG("[shell] timer finished, alert\n");
}

void Shell::dismissAlert(Context& ctx) {
  alert_ = false;
  if (buzzer_) buzzer_->stop();
  show(Screen::Eyes, ctx);                    // agreed rule: dismiss always lands on the eyes
}

void Shell::drawAlert(Context& ctx, Adafruit_GFX& g) {
  const bool flashOn = ((ctx.nowMs - alertStartMs_) / cfg::CLOCK_BLINK_HALF_MS) % 2 == 0;
  if (flashOn) g.fillRect(0, 0, cfg::SCREEN_W, cfg::SCREEN_H, cfg::C_ON);   // the flash itself
  const uint16_t ink = flashOn ? cfg::C_OFF : cfg::C_ON;
  ui::textCentered(g, 10, "TIME'S UP", ink, true);
  g.setTextSize(2); g.setTextColor(ink); g.setTextWrap(false);
  g.setCursor(16, 26); g.print("00:00:00");
  g.setTextSize(1);
  ui::textCentered(g, 52, "CENTER or EXIT", ink);
}
