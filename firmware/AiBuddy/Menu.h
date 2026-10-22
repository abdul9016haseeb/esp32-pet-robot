// Menu.h - the apps list: selection, scrolling and drawing. No input handling, no hardware.
#pragma once
#include <Adafruit_GFX.h>
#include "Icons.h"

class App;

struct MenuEntry {
  const char* label;
  icons::Id   icon;
  App*        app;       // nullptr = the "EXIT" row: back to the eyes
};

class Menu {
 public:
  void setEntries(const MenuEntry* entries, uint8_t count);   // array must outlive the menu
  void move(int dir);                                         // -1 = up, +1 = down, wraps around
  const MenuEntry* selected() const { return count_ ? &entries_[sel_] : nullptr; }
  void draw(Adafruit_GFX& g) const;

 private:
  void drawTitle(Adafruit_GFX& g) const;
  void drawRow(Adafruit_GFX& g, uint8_t index, int y) const;
  void drawScrollbar(Adafruit_GFX& g) const;

  const MenuEntry* entries_ = nullptr;
  uint8_t count_ = 0, sel_ = 0, top_ = 0;     // top_ = index of the first visible row
};
