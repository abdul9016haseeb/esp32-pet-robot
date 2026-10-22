// App.h - what the Shell runs when the user picks a menu entry.
// An App owns the screen while it is running. The Shell handles EXIT; the app gets the other
// five buttons. To add an app: derive from App, create one global object, add one MenuEntry.
#pragma once
#include <Adafruit_GFX.h>
#include "Feature.h"
#include "Ui.h"

class App {
 public:
  virtual ~App() {}
  virtual const char* name() const = 0;
  virtual void onEnter(Context&) {}                        // menu -> app
  virtual void onExit(Context&) {}                         // app -> menu (EXIT button)
  virtual void update(Context&) {}                         // every simulation tick while running
  virtual void onButton(Context&, Button, bool /*pressed*/) {}   // UP/DOWN/LEFT/RIGHT/CENTER only
  virtual void draw(Context&, Adafruit_GFX&) = 0;          // every render while running
};

// Placeholder shown until a real app exists.
class StubApp : public App {
 public:
  explicit StubApp(const char* title) : title_(title) {}
  const char* name() const override { return title_; }
  void draw(Context&, Adafruit_GFX& g) override {
    ui::textCentered(g, 14, title_, cfg::C_ON, true);
    ui::textCentered(g, 32, "Coming soon", cfg::C_ON);
    ui::textCentered(g, 52, "EXIT = back", cfg::C_ON);
  }
 private:
  const char* title_;
};
