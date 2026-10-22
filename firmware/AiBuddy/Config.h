// Config.h - every tunable number lives here. Nothing else in the project hardcodes a constant.
#pragma once
#include <Arduino.h>
#include <stdint.h>

#define BUDDY_DEBUG 1           // 1 = serial telemetry + state-change logs
#if BUDDY_DEBUG
  #define BUDDY_LOG(...) Serial.printf(__VA_ARGS__)
#else
  #define BUDDY_LOG(...) do {} while (0)
#endif

namespace cfg {

// ---------------- Hardware ----------------
constexpr uint8_t  PIN_SDA   = 21;
constexpr uint8_t  PIN_SCL   = 22;
constexpr uint32_t I2C_HZ    = 400000;
constexpr uint8_t  OLED_ADDR = 0x3C;
constexpr uint8_t  MPU_ADDR  = 0x68;
constexpr int      SCREEN_W  = 128;
constexpr int      SCREEN_H  = 64;
constexpr uint16_t C_ON      = 1;   // white pixel
constexpr uint16_t C_OFF     = 0;

// ---------------- Buttons (wired to GND, INPUT_PULLUP, no resistors) ----------------
// GPIO 16/17 (RX2/TX2) are plain GPIOs as long as Serial2 is never started.
constexpr uint8_t  PIN_BTN_UP     = 19;
constexpr uint8_t  PIN_BTN_DOWN   = 18;
constexpr uint8_t  PIN_BTN_LEFT   = 17;
constexpr uint8_t  PIN_BTN_RIGHT  = 16;
constexpr uint8_t  PIN_BTN_CENTER = 4;
constexpr uint8_t  PIN_BTN_EXIT   = 23;
constexpr uint32_t BTN_DEBOUNCE_MS = 30;   // a level must hold this long to count; shorter blips are bounce

// ---------------- Apps menu ----------------
constexpr const char* MENU_TITLE = "ESP32 APPS";
constexpr bool MENU_POLISHED   = true;     // true = clean look (bold title, chevron, bold selection)
                                           // false = classic look (boxed title, inverted selection bar)
constexpr int  MENU_TITLE_H      = 12;     // title area height [px]
constexpr int  MENU_TOP          = 14;     // y of the first row
constexpr int  MENU_ROW_H        = 12;     // 4 rows * 12 px = 48 px -> ends at y = 62
constexpr int  MENU_VISIBLE_ROWS = 4;      // more entries than this -> list scrolls + scrollbar appears
constexpr int  MENU_CHEVRON_X    = 2;
constexpr int  MENU_ICON_X       = 12;
constexpr int  MENU_TEXT_X       = 28;
constexpr int  MENU_BAR_X        = 10;     // classic style: inverted selection bar
constexpr int  MENU_BAR_W        = 112;

// ---------------- Mounting: sensor axes -> screen axes ----------------
// Screen X = right, screen Y = DOWN. Axis index: 0 = sensor X, 1 = sensor Y, 2 = sensor Z.
// Check: hold the device upright; the eyes should sag slightly DOWN. If they sag up/sideways,
// change the axis or flip the sign here (raw reading is multiplied by the sign).
constexpr uint8_t AXIS_SCREEN_X = 1;  constexpr int8_t SIGN_SCREEN_X = +1;
constexpr uint8_t AXIS_SCREEN_Y = 0;  constexpr int8_t SIGN_SCREEN_Y = +1;

// ---------------- Scheduler ----------------
constexpr uint32_t SIM_PERIOD_MS    = 10;   // IMU read + physics tick (~100 Hz target)
constexpr uint32_t RENDER_PERIOD_MS = 40;   // 25 fps (a full OLED refresh takes ~23 ms at 400 kHz)
constexpr float    MAX_DT           = 0.05f;

// ---------------- IMU ----------------
constexpr float ACC_LSB_PER_G     = 8192.0f;  // +/-4 g range
constexpr float GYRO_LSB_PER_DPS  = 65.5f;    // +/-500 dps range
constexpr int   CAL_SAMPLES       = 100;
constexpr float CAL_MAX_RANGE_G   = 0.12f;    // if the board moved more than this, calibration is rejected

// ---------------- Motion detection ----------------
constexpr float    SHAKE_THRESH_G   = 0.35f;  // from your bench test
constexpr uint8_t  SHAKE_PEAKS      = 3;
constexpr uint32_t SHAKE_WINDOW_MS  = 700;
constexpr uint32_t SHAKE_END_MS     = 450;    // no peak for this long = shake finished
constexpr float    FF_ENTER_G       = 0.35f;  // |a| below this = weightless
constexpr float    FF_EXIT_G        = 0.60f;  // hysteresis
constexpr uint32_t FF_MIN_MS        = 70;
constexpr uint32_t IMPACT_WINDOW_MS = 120;
constexpr float    IMPACT_G         = 1.8f;
constexpr uint32_t SHAKE_SUPPRESS_AFTER_FF_MS = 300;

// ---------------- Eye geometry (pixels) ----------------
constexpr float EYE_W        = 32.f;
constexpr float EYE_H        = 34.f;
constexpr float EYE_RADIUS   = 9.f;
constexpr float EYE_SPACING  = 60.f;   // distance between eye centres
constexpr float EYE_CENTER_Y = 32.f;
constexpr float LIMIT_X      = 20.f;   // how far an eye may travel before it hits the "wall"
constexpr float LIMIT_Y      = 17.f;

// ---------------- Eye physics ----------------
constexpr float EYE_STIFFNESS   = 480.f;   // spring k (natural frequency ~3.5 Hz)
constexpr float EYE_DAMPING     = 14.f;    // damping c (underdamped: eyes overshoot and wobble)
constexpr float EYE_RESTITUTION = 0.45f;   // bounce off the limits
constexpr float GRAVITY_GAIN    = 8000.f;  // px/s^2 per g; a 90-degree tilt from home moves the eyes ~12.5 px
constexpr float REST_ADAPT_TAU_S = 30.f;   // home orientation slowly follows the real one (0 = never)
constexpr float FLOAT_DRIFT_ACCEL = 260.f; // zero-g drift strength
constexpr float TREMBLE_ACCEL   = 900.f;
constexpr float SQUASH_K        = 260.f;
constexpr float SQUASH_C        = 13.f;
constexpr float DIZZY_ORBIT_PX  = 5.f;
constexpr float DIZZY_SPIN_RAD_S = 7.f;
constexpr float BLINK_SECONDS   = 0.16f;
constexpr uint32_t BLINK_MIN_MS = 2200;
constexpr uint32_t BLINK_MAX_MS = 5500;

// ---------------- Behaviour ----------------
constexpr float HARD_SHAKE_INTENSITY_G = 0.8f;  // mean peak deviation of a shake: >= this -> dizzy (spiral), below -> angry
constexpr float ANGRY_PER_SECOND    = 0.5f;  // extra angry time per second of shaking
constexpr float DIZZY_BASE_S        = 1.5f;
constexpr float DIZZY_PER_ENERGY    = 0.25f;
constexpr float DIZZY_MAX_S         = 6.f;
constexpr float ANGRY_BASE_S        = 3.0f;
constexpr float ANGRY_MAX_S         = 6.f;
constexpr float MIN_FALL_S          = 0.15f; // shorter "falls" are ignored for the landing reaction
constexpr float LANDING_WAIT_S      = 0.4f;
constexpr float DIZZY_FALL_S        = 0.6f;
constexpr float DIZZY_IMPACT_G      = 3.5f;
constexpr float SPIN_DIZZY_DPS      = 360.f; // sustained spinning makes it dizzy
constexpr float SPIN_DIZZY_AFTER_S  = 0.6f;
constexpr float GLANCE_SPEED        = 140.f; // idle look-around impulse, px/s
constexpr uint32_t GLANCE_MIN_MS    = 2500;
constexpr uint32_t GLANCE_MAX_MS    = 6000;

// ---------------- Clock app (ClockService + ClockApp) ----------------
constexpr int      CLOCK_TAB_H         = 12;    // top tab-bar height [px]
constexpr uint32_t CLOCK_BLINK_HALF_MS = 500;   // half-period of the flashing colon / TIME'S UP / alert flash

// Countdown timer + time edit
constexpr uint32_t TIMER_MAX_SEC         = 5999;  // 99:59
constexpr int      TIMER_ADJUST_STEP_SEC = 1;     // seconds per UP/DOWN step
constexpr uint32_t BTN_REPEAT_DELAY_MS   = 400;   // hold this long before auto-repeat starts
constexpr uint32_t BTN_REPEAT_PERIOD_MS  = 150;   // then repeat at this cadence (~6.7 steps/s)

// Timer-finished alert (Shell takeover)
constexpr uint32_t ALERT_LOCKOUT_MS      = 800;    // all buttons ignored for the first moments of an alert
constexpr uint32_t ALERT_BUZZ_HALF_MS    = 250;    // buzzer beep on/off half-period while alerting
constexpr uint32_t ALERT_BUZZ_TIMEOUT_MS = 60000;  // the buzzer gives up after 1 min; the screen stays up

// Buzzer: OPTIONAL active buzzer on the spare pin. Leave BUZZER_ENABLED = false until
// one is really wired there - the alert flashes the screen instead. If the buzzer
// draws more than ~10 mA, drive it through a small NPN transistor, not the GPIO.
// NOTE: D5 glitches HIGH briefly at boot, so a wired buzzer chirps once at power-up.
constexpr bool     BUZZER_ENABLED = false;
constexpr uint8_t  PIN_BUZZER     = 5;

}  // namespace cfg
