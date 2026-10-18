// Shell.h - screen switching: Eyes <-> Menu <-> App. Owns the menu and the running app.
//
//   Eyes --CENTER--> Menu --CENTER on an app--> App
//   App  --EXIT----> Menu --EXIT or EXIT row--> Eyes
//
// The Shell reads EXIT before any app sees it, so apps never handle exit themselves.
#pragma once
#include <Adafruit_GFX.h>
#include "Feature.h"
#include "Menu.h"
#include "App.h"

class Shell {
 public:
  void begin(const MenuEntry* entries, uint8_t count) { menu_.setEntries(entries, count); }
  bool onEvent(Context& ctx, const Event& e);        // true = consumed, do not pass to Features
  void update(Context& ctx);                         // every simulation tick
  void draw(Context& ctx, Adafruit_GFX& g);          // every render, after the eyes
  Screen screen() const { return screen_; }

 private:
  void show(Screen s, Context& ctx);
  void onMenuButton(Context& ctx, Button b);
  void launch(App* app, Context& ctx);
  void closeApp(Context& ctx);
  void routeToApp(Context& ctx, Button b, bool pressed);

  Menu    menu_;
  Screen  screen_ = Screen::Eyes;
  App*    active_ = nullptr;
  uint8_t held_   = 0;      // buttons pressed while the app was running (bit per Button)
};
