// Buttons.h - debounced push buttons -> ButtonPressed / ButtonReleased events.
// Hardware access only; knows nothing about menus or apps.
#pragma once
#include "Types.h"

class Buttons {
 public:
  void begin();                                   // sets INPUT_PULLUP, adopts the current levels
  void poll(uint32_t nowMs, EventQueue& q);       // call every simulation tick
  bool isDown(Button b) const { return s_[(uint8_t)b].down; }

 private:
  struct State {
    bool     raw = false;        // last instantaneous reading (true = pressed)
    bool     down = false;       // debounced state
    uint32_t changedMs = 0;      // when raw last flipped
    uint32_t pressedMs = 0;      // when the debounced press happened
  };
  State s_[(uint8_t)Button::Count];
};
