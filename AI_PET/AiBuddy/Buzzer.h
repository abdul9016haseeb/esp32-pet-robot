// Buzzer.h - non-blocking beeper for an ACTIVE buzzer (beeps on plain HIGH).
// All timing is driven by update(nowMs) from the Shell; there is no delay()
// anywhere. cfg::BUZZER_ENABLED = false turns every method into a no-op, so the
// firmware builds and runs without the part - the alert still flashes the screen.
//
// Wiring: PIN_BUZZER -> buzzer (+), buzzer (-) -> GND. If the buzzer draws more
// than ~10 mA, drive it through a small NPN transistor instead of the GPIO.
// NOTE: D5 glitches HIGH for a moment at boot, so a wired buzzer chirps once at
// power-up. Harmless; mentioned so it does not surprise you during the bench test.
#pragma once
#include <Arduino.h>
#include "Config.h"

class Buzzer {
 public:
  void begin() {
    if (!cfg::BUZZER_ENABLED) return;
    pinMode(cfg::PIN_BUZZER, OUTPUT);
    digitalWrite(cfg::PIN_BUZZER, LOW);
  }

  void start(uint32_t now) {                 // begin the alert beeping
    if (!cfg::BUZZER_ENABLED) return;
    active_ = true; timedOut_ = false; pinOn_ = false;
    startedMs_ = now; nextToggleMs_ = now;
  }

  void stop() {                              // immediate silence
    active_ = false;
    if (cfg::BUZZER_ENABLED) digitalWrite(cfg::PIN_BUZZER, LOW);
    pinOn_ = false;
  }

  void update(uint32_t now) {                // call every tick while alerting
    if (!cfg::BUZZER_ENABLED || !active_) return;
    if (!timedOut_ && (uint32_t)(now - startedMs_) >= cfg::ALERT_BUZZ_TIMEOUT_MS) {
      timedOut_ = true;                      // give up beeping, the screen stays up
      digitalWrite(cfg::PIN_BUZZER, LOW);
      pinOn_ = false;
      return;
    }
    if (timedOut_) return;
    if ((int32_t)(now - nextToggleMs_) >= 0) {
      pinOn_ = !pinOn_;
      digitalWrite(cfg::PIN_BUZZER, pinOn_ ? HIGH : LOW);
      nextToggleMs_ = now + cfg::ALERT_BUZZ_HALF_MS;
    }
  }

 private:
  bool     active_ = false, timedOut_ = false, pinOn_ = false;
  uint32_t startedMs_ = 0, nextToggleMs_ = 0;
};
