#include "Shell.h"
#include "Config.h"

void Shell::show(Screen s, Context& ctx) {
  screen_ = s;
  ctx.screen = s;                      // later events in the same tick see the new screen
  BUDDY_LOG("[shell] -> %s\n", s == Screen::Eyes ? "eyes" : (s == Screen::Menu ? "menu" : "app"));
}

bool Shell::onEvent(Context& ctx, const Event& e) {
  if (e.type != EventType::ButtonPressed && e.type != EventType::ButtonReleased) return false;
  const Button b = buttonOf(e);
  const bool pressed = e.type == EventType::ButtonPressed;

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
  if (screen_ == Screen::App && active_) active_->update(ctx);
}

void Shell::draw(Context& ctx, Adafruit_GFX& g) {
  if (screen_ == Screen::Menu)                     menu_.draw(g);
  else if (screen_ == Screen::App && active_)      active_->draw(ctx, g);
}
