// Ui.h - tiny drawing helpers shared by the menu and the apps. No state.
#pragma once
#include <Adafruit_GFX.h>
#include <string.h>
#include "Config.h"

namespace ui {

constexpr int CHAR_W = 6, CHAR_H = 8;     // Adafruit GFX built-in font at text size 1

inline int textWidth(const char* s) { return (int)strlen(s) * CHAR_W; }

// bold = the text is drawn twice, 1 px apart (the built-in font has no bold face).
inline void text(Adafruit_GFX& g, int x, int y, const char* s, uint16_t color, bool bold = false) {
  g.setTextSize(1);
  g.setTextColor(color);
  g.setTextWrap(false);
  g.setCursor(x, y);
  g.print(s);
  if (bold) { g.setCursor(x + 1, y); g.print(s); }
}

inline void textCentered(Adafruit_GFX& g, int y, const char* s, uint16_t color, bool bold = false) {
  text(g, (cfg::SCREEN_W - textWidth(s)) / 2, y, s, color, bold);
}

}  // namespace ui
