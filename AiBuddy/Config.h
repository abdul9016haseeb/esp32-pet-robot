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

}  // namespace cfg