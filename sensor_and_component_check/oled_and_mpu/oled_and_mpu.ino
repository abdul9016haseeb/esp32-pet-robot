#include <Wire.h>
#include <math.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

// ---------- Confirmed hardware ----------
constexpr uint8_t SDA_PIN = 21;
constexpr uint8_t SCL_PIN = 22;
constexpr uint8_t OLED_ADDR = 0x3C;
constexpr uint8_t MPU_ADDR = 0x68;

constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int OLED_RESET = -1;

// ---------- Sensor configuration ----------
constexpr uint32_t SENSOR_PERIOD_MS = 10;  // 100 Hz
constexpr uint8_t MPU_WHO_AM_I_REG = 0x75;

// Your standalone-tested shake settings
constexpr float SHAKE_THRESHOLD_G = 0.35f;
constexpr float SHAKE_RELEASE_G = 0.20f;   // hysteresis to prevent noisy re-triggering
constexpr uint8_t SHAKE_PEAKS_REQUIRED = 3;
constexpr uint32_t SHAKE_WINDOW_MS = 700;
constexpr uint32_t SHAKE_LOCKOUT_MS = 1500;
constexpr uint32_t SHAKE_REARM_STILL_MS = 180;
constexpr uint32_t SHAKE_REACTION_MS = 1800;

// Starting points for free-fall/landing; tune after safe, mounted testing
constexpr float FREEFALL_THRESHOLD_G = 0.35f;
constexpr float FREEFALL_EXIT_G = 0.65f;
constexpr float HARD_LANDING_THRESHOLD_G = 1.65f;
constexpr uint32_t FREEFALL_MIN_MS = 120;
constexpr uint32_t FREEFALL_MAX_MS = 1800;
constexpr uint32_t FREEFALL_REARM_STILL_MS = 180;
constexpr uint32_t SETTLE_TIME_MS = 600;

// ---------- Display and eyes ----------
Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET
);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

bool oledReady = false;

// ---------- Sensor data ----------
float baselineMagnitudeG = 1.0f;
float calibrationStdDevG = 0.0f;
float accelMagnitudeG = 1.0f;
float gyroMagnitudeDps = 0.0f;

// ---------- Buddy behavior state ----------
enum class BuddyMode : uint8_t {
  IDLE,
  FREEFALL,
  SHAKE_REACTION,
  SETTLING
};

BuddyMode mode = BuddyMode::IDLE;
uint32_t modeStartedMs = 0;

// Shake detector state
bool shakeArmed = true;
bool shakeAboveThreshold = false;
bool shakeCalmTimerActive = false;
uint32_t shakeCalmSinceMs = 0;
uint32_t shakeLockUntilMs = 0;
uint32_t shakeWindowStartedMs = 0;
uint8_t shakePeakCount = 0;
float shakeWindowMaxDeviationG = 0.0f;

// Free-fall detector state
bool freeFallArmed = true;
bool freeFallCandidateActive = false;
bool freeFallNormalTimerActive = false;
uint32_t freeFallCandidateSinceMs = 0;
uint32_t freeFallNormalSinceMs = 0;
uint32_t lastFloatGazeMs = 0;
uint8_t floatGazeIndex = 0;
bool settleEyesCentered = false;

// Timers
uint32_t lastSensorReadMs = 0;
uint32_t lastTelemetryMs = 0;
uint32_t lastReadErrorMs = 0;

// ---------- Small utilities ----------
bool timeReached(uint32_t now, uint32_t deadline) {
  return (int32_t)(now - deadline) >= 0;
}

bool elapsedAtLeast(uint32_t now, uint32_t start, uint32_t duration) {
  return (uint32_t)(now - start) >= duration;
}

bool i2cDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void showMessage(const char *line1, const char *line2) {
  if (!oledReady) return;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 12);
  display.println(line1);
  display.setCursor(0, 28);
  display.println(line2);
  display.display();
}

void fatalError(const char *message) {
  Serial.println(message);
  showMessage("Hardware error:", message);

  while (true) {
    delay(1000);
  }
}

// ---------- MPU6050 register access ----------
bool writeMpuRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readMpuRegister(uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)1) != 1) {
    return false;
  }

  value = Wire.read();
  return true;
}

int16_t combineBytes(uint8_t highByte, uint8_t lowByte) {
  return (int16_t)(((uint16_t)highByte << 8) | lowByte);
}

bool readSensorFrame(float &accelMagG, float &gyroMagDps) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);  // ACCEL_XOUT_H

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)14) != 14) {
    while (Wire.available()) Wire.read();
    return false;
  }

  uint8_t b[14];
  for (uint8_t i = 0; i < 14; i++) {
    b[i] = Wire.read();
  }

  const int16_t axRaw = combineBytes(b[0], b[1]);
  const int16_t ayRaw = combineBytes(b[2], b[3]);
  const int16_t azRaw = combineBytes(b[4], b[5]);

  const int16_t gxRaw = combineBytes(b[8], b[9]);
  const int16_t gyRaw = combineBytes(b[10], b[11]);
  const int16_t gzRaw = combineBytes(b[12], b[13]);

  // Configured below for accelerometer ±2 g and gyro ±250 degrees/second.
  const float axG = axRaw / 16384.0f;
  const float ayG = ayRaw / 16384.0f;
  const float azG = azRaw / 16384.0f;

  const float gxDps = gxRaw / 131.0f;
  const float gyDps = gyRaw / 131.0f;
  const float gzDps = gzRaw / 131.0f;

  accelMagG = sqrtf(axG * axG + ayG * ayG + azG * azG);
  gyroMagDps = sqrtf(gxDps * gxDps + gyDps * gyDps + gzDps * gzDps);
  return true;
}

bool initializeMpu6050() {
  // Wake the sensor.
  if (!writeMpuRegister(0x6B, 0x00)) return false;
  delay(100);

  uint8_t identity = 0;
  if (!readMpuRegister(MPU_WHO_AM_I_REG, identity) || identity != 0x68) {
    Serial.printf("MPU identity response: 0x%02X\n", identity);
    return false;
  }

  // DLPF setting 3; accelerometer ±2 g; gyro ±250 dps.
  // Sample-rate divider 9 gives approximately 100 Hz with the DLPF enabled.
  if (!writeMpuRegister(0x1A, 0x03)) return false;
  if (!writeMpuRegister(0x1B, 0x00)) return false;
  if (!writeMpuRegister(0x1C, 0x00)) return false;
  if (!writeMpuRegister(0x19, 0x09)) return false;

  delay(50);
  return true;
}

// ---------- Still-start calibration ----------
bool calibrateAtRest() {
  constexpr uint16_t SAMPLE_COUNT = 200;
  double sum = 0.0;
  double sumSquares = 0.0;
  uint16_t goodSamples = 0;

  Serial.println("Keep the robot still for 2 seconds...");
  showMessage("Calibrating...", "Keep it still");

  for (uint16_t i = 0; i < SAMPLE_COUNT; i++) {
    float magG, gyroDps;

    if (readSensorFrame(magG, gyroDps)) {
      sum += magG;
      sumSquares += (double)magG * (double)magG;
      goodSamples++;
    }

    delay(10);
  }

  if (goodSamples < 190) {
    Serial.printf("Calibration failed: only %u samples\n", goodSamples);
    return false;
  }

  const double mean = sum / goodSamples;
  double variance = (sumSquares / goodSamples) - (mean * mean);
  if (variance < 0.0) variance = 0.0;

  baselineMagnitudeG = (float)mean;
  calibrationStdDevG = sqrtf((float)variance);

  Serial.printf(
    "Baseline: %.3f g, standard deviation: %.3f g\n",
    baselineMagnitudeG,
    calibrationStdDevG
  );

  // Reject a moving or implausible startup calibration rather than silently
  // learning a bad baseline.
  if (baselineMagnitudeG < 0.75f || baselineMagnitudeG > 1.25f ||
      calibrationStdDevG > 0.08f) {
    Serial.println("Calibration rejected. Reset and keep the robot still.");
    return false;
  }

  return true;
}

// ---------- Eye state transitions ----------
void enterIdle(uint32_t now) {
  mode = BuddyMode::IDLE;
  modeStartedMs = now;
  freeFallCandidateActive = false;
  settleEyesCentered = false;

  roboEyes.setMood(DEFAULT);
  roboEyes.setPosition(DEFAULT);
  roboEyes.setIdleMode(ON, 2, 2);
}

void enterShakeReaction(uint32_t now, const char *reason) {
  mode = BuddyMode::SHAKE_REACTION;
  modeStartedMs = now;
  freeFallCandidateActive = false;
  shakePeakCount = 0;
  shakeWindowMaxDeviationG = 0.0f;

  roboEyes.setIdleMode(OFF, 2, 2);
  roboEyes.setMood(ANGRY);
  roboEyes.setPosition(DEFAULT);
  roboEyes.anim_confused();

  Serial.printf("%s reaction\n", reason);
}

void enterFreeFall(uint32_t now) {
  mode = BuddyMode::FREEFALL;
  modeStartedMs = now;
  lastFloatGazeMs = now;
  floatGazeIndex = 0;
  freeFallCandidateActive = false;
  freeFallArmed = false;
  freeFallNormalTimerActive = false;
  shakePeakCount = 0;
  shakeWindowMaxDeviationG = 0.0f;

  roboEyes.setIdleMode(OFF, 2, 2);
  roboEyes.setMood(DEFAULT);
  roboEyes.setPosition(N);

  Serial.println("Free-fall-like acceleration detected");
}

void enterSettling(uint32_t now) {
  mode = BuddyMode::SETTLING;
  modeStartedMs = now;
  settleEyesCentered = false;

  roboEyes.setIdleMode(OFF, 2, 2);
  roboEyes.setMood(DEFAULT);
  roboEyes.setPosition(S);

  Serial.println("Free-fall ended; settling");
}

// ---------- Shake detection ----------
void updateShakeRearming(uint32_t now, float deviationG) {
  if (shakeArmed) return;

  if (!timeReached(now, shakeLockUntilMs) ||
      deviationG > SHAKE_RELEASE_G) {
    shakeCalmTimerActive = false;
    return;
  }

  if (!shakeCalmTimerActive) {
    shakeCalmTimerActive = true;
    shakeCalmSinceMs = now;
    return;
  }

  if (elapsedAtLeast(now, shakeCalmSinceMs, SHAKE_REARM_STILL_MS)) {
    shakeArmed = true;
    shakeCalmTimerActive = false;
    shakePeakCount = 0;
    shakeWindowMaxDeviationG = 0.0f;
  }
}

void registerShakePeak(uint32_t now, float deviationG) {
  if (shakePeakCount == 0 ||
      elapsedAtLeast(now, shakeWindowStartedMs, SHAKE_WINDOW_MS + 1)) {
    shakePeakCount = 0;
    shakeWindowStartedMs = now;
    shakeWindowMaxDeviationG = 0.0f;
  }

  shakePeakCount++;
  if (deviationG > shakeWindowMaxDeviationG) {
    shakeWindowMaxDeviationG = deviationG;
  }

  if (shakePeakCount >= SHAKE_PEAKS_REQUIRED &&
      timeReached(now, shakeLockUntilMs)) {
    Serial.printf(
      "Shake: max deviation %.2f g; gyro magnitude %.0f dps\n",
      shakeWindowMaxDeviationG,
      gyroMagnitudeDps
    );

    shakeArmed = false;
    shakeLockUntilMs = now + SHAKE_LOCKOUT_MS;
    shakeCalmTimerActive = false;
    enterShakeReaction(now, "Shake");
  }
}

void processShakeDetector(uint32_t now, float magG) {
  const float deviationG = fabsf(magG - baselineMagnitudeG);

  updateShakeRearming(now, deviationG);

  const bool newPeak =
    (!shakeAboveThreshold && deviationG >= SHAKE_THRESHOLD_G);

  if (deviationG <= SHAKE_RELEASE_G) {
    shakeAboveThreshold = false;
  } else if (newPeak) {
    shakeAboveThreshold = true;
  }

  // Do not collect peaks while reacting to another event.
  if (mode != BuddyMode::IDLE) return;

  if (newPeak && shakeArmed && timeReached(now, shakeLockUntilMs)) {
    registerShakePeak(now, deviationG);
  }

  if (shakePeakCount > 0 &&
      elapsedAtLeast(now, shakeWindowStartedMs, SHAKE_WINDOW_MS + 1)) {
    shakePeakCount = 0;
    shakeWindowMaxDeviationG = 0.0f;
  }
}

// ---------- Free-fall detection ----------
void updateFreeFallRearming(uint32_t now, float magG) {
  if (freeFallArmed) return;

  if (magG < FREEFALL_EXIT_G) {
    freeFallNormalTimerActive = false;
    return;
  }

  if (!freeFallNormalTimerActive) {
    freeFallNormalTimerActive = true;
    freeFallNormalSinceMs = now;
    return;
  }

  if (elapsedAtLeast(now, freeFallNormalSinceMs,
                     FREEFALL_REARM_STILL_MS)) {
    freeFallArmed = true;
    freeFallNormalTimerActive = false;
  }
}

void processFreeFallCandidate(uint32_t now, float magG) {
  if (mode != BuddyMode::IDLE || !freeFallArmed) {
    freeFallCandidateActive = false;
    return;
  }

  if (magG <= FREEFALL_THRESHOLD_G) {
    if (!freeFallCandidateActive) {
      freeFallCandidateActive = true;
      freeFallCandidateSinceMs = now;
    } else if (elapsedAtLeast(now, freeFallCandidateSinceMs,
                              FREEFALL_MIN_MS)) {
      enterFreeFall(now);
    }
  } else {
    freeFallCandidateActive = false;
  }
}

// ---------- Behavior update ----------
void updateBuddyMode(uint32_t now, float magG) {
  switch (mode) {
    case BuddyMode::IDLE:
      break;

    case BuddyMode::FREEFALL: {
      const uint32_t freeFallDuration = now - modeStartedMs;

      // Interpret a high-g impulse as a hard landing only after a
      // free-fall-like period has already been detected.
      if (magG >= HARD_LANDING_THRESHOLD_G &&
          freeFallDuration <= FREEFALL_MAX_MS) {
        enterShakeReaction(now, "Hard landing");
        break;
      }

      if (magG >= FREEFALL_EXIT_G ||
          freeFallDuration >= FREEFALL_MAX_MS) {
        enterSettling(now);
        break;
      }

      // Weightless-looking gaze drift: a visual effect, not a claim that
      // the sensor knows the robot's absolute position or direction.
      if (elapsedAtLeast(now, lastFloatGazeMs, 450)) {
        static const uint8_t gazeSequence[] = {NE, N, NW, N};
        constexpr uint8_t sequenceLength =
          sizeof(gazeSequence) / sizeof(gazeSequence[0]);

        roboEyes.setPosition(gazeSequence[floatGazeIndex]);
        floatGazeIndex = (floatGazeIndex + 1) % sequenceLength;
        lastFloatGazeMs = now;
      }
      break;
    }

    case BuddyMode::SHAKE_REACTION:
      if (elapsedAtLeast(now, modeStartedMs, SHAKE_REACTION_MS)) {
        enterIdle(now);
      }
      break;

    case BuddyMode::SETTLING:
      if (!settleEyesCentered &&
          elapsedAtLeast(now, modeStartedMs, 250)) {
        roboEyes.setPosition(DEFAULT);
        settleEyesCentered = true;
      }

      if (elapsedAtLeast(now, modeStartedMs, SETTLE_TIME_MS)) {
        enterIdle(now);
      }
      break;
  }
}

const char *modeName() {
  switch (mode) {
    case BuddyMode::IDLE: return "IDLE";
    case BuddyMode::FREEFALL: return "FREEFALL";
    case BuddyMode::SHAKE_REACTION: return "REACTION";
    case BuddyMode::SETTLING: return "SETTLING";
    default: return "?";
  }
}

// ---------- Arduino setup/loop ----------
void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Serial.println("I2C scan:");
  const bool oledFound = i2cDevicePresent(OLED_ADDR);
  const bool mpuFound = i2cDevicePresent(MPU_ADDR);

  Serial.printf("OLED 0x%02X: %s\n",
                OLED_ADDR, oledFound ? "found" : "not found");
  Serial.printf("MPU  0x%02X: %s\n",
                MPU_ADDR, mpuFound ? "found" : "not found");

  if (!oledFound) fatalError("OLED missing at 0x3C");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    fatalError("OLED initialization failed");
  }
  oledReady = true;

  if (!mpuFound) fatalError("MPU missing at 0x68");
  if (!initializeMpu6050()) fatalError("MPU setup/identity failed");

  if (!calibrateAtRest()) {
    fatalError("Calibration failed; reset while still");
  }

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 30);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);
  roboEyes.setMood(DEFAULT);
  roboEyes.setPosition(DEFAULT);

  mode = BuddyMode::IDLE;
  modeStartedMs = millis();

  Serial.println("Buddy ready. Shake it gently to test.");
}

void loop() {
  // Keep the OLED animation updating continuously.
  roboEyes.update();

  const uint32_t now = millis();
  if (!elapsedAtLeast(now, lastSensorReadMs, SENSOR_PERIOD_MS)) {
    return;
  }
  lastSensorReadMs = now;

  if (!readSensorFrame(accelMagnitudeG, gyroMagnitudeDps)) {
    if (elapsedAtLeast(now, lastReadErrorMs, 1000)) {
      Serial.println("MPU read failed");
      lastReadErrorMs = now;
    }
    return;  // Failed reads never count as zero-g.
  }

  updateFreeFallRearming(now, accelMagnitudeG);
  processFreeFallCandidate(now, accelMagnitudeG);
  processShakeDetector(now, accelMagnitudeG);
  updateBuddyMode(now, accelMagnitudeG);

  if (elapsedAtLeast(now, lastTelemetryMs, 250)) {
    const float deviationG =
      fabsf(accelMagnitudeG - baselineMagnitudeG);

    Serial.printf(
      "mode=%s | accel=%.2f g | deviation=%.2f g | gyro=%.0f dps\n",
      modeName(),
      accelMagnitudeG,
      deviationG,
      gyroMagnitudeDps
    );

    lastTelemetryMs = now;
  }
}
