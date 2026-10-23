#include "Buttons.h"
#include "Config.h"

namespace {
// Same order as enum class Button.
const uint8_t PINS[(uint8_t)Button::Count] = {
  cfg::PIN_BTN_UP, cfg::PIN_BTN_DOWN, cfg::PIN_BTN_LEFT,
  cfg::PIN_BTN_RIGHT, cfg::PIN_BTN_CENTER, cfg::PIN_BTN_EXIT
};
#if BUDDY_DEBUG
const char* const NAMES[(uint8_t)Button::Count] = {"UP", "DOWN", "LEFT", "RIGHT", "CENTER", "EXIT"};
#endif
inline bool readPressed(uint8_t pin) { return digitalRead(pin) == LOW; }   // wired to GND
}

void Buttons::begin() {
  for (uint8_t i = 0; i < (uint8_t)Button::Count; i++) pinMode(PINS[i], INPUT_PULLUP);
  delay(5);                                       // let the pull-ups settle
  const uint32_t now = millis();
  for (uint8_t i = 0; i < (uint8_t)Button::Count; i++) {
    s_[i].raw = s_[i].down = readPressed(PINS[i]);   // a button already held at boot is not a "press"
    s_[i].changedMs = now;
  }
}

// A level change is accepted only after it has stayed stable for BTN_DEBOUNCE_MS. Presses shorter
// than that never become stable, so they produce no event at all (that is the "ignore < 30 ms" rule).
void Buttons::poll(uint32_t now, EventQueue& q) {
  for (uint8_t i = 0; i < (uint8_t)Button::Count; i++) {
    State& s = s_[i];
    const bool raw = readPressed(PINS[i]);
    if (raw != s.raw) { s.raw = raw; s.changedMs = now; }
    if (raw == s.down || now - s.changedMs < cfg::BTN_DEBOUNCE_MS) continue;

    const Button b = (Button)i;
    const float heldS = raw ? 0.f : (now - s.pressedMs) * 0.001f;
    const Event ev = makeButtonEvent(raw ? EventType::ButtonPressed : EventType::ButtonReleased, b, heldS, now);
    if (!q.push(ev)) continue;                    // queue full: keep the old state and retry next tick
    s.down = raw;
    if (raw) s.pressedMs = now;
    BUDDY_LOG("[btn] %s %s (held %.2f s)\n", NAMES[i], raw ? "pressed" : "released", heldS);
  }
}
