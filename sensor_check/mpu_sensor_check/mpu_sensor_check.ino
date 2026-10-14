#include <Wire.h>

#define SDA_PIN 21
#define SCL_PIN 22
const uint8_t MPU_ADDR = 0x68;

// Tunables
const float    THRESH   = 0.35;  // g deviation from baseline that counts as a peak
const int      PEAKS    = 3;     // peaks needed within the window
const uint32_t WINDOW   = 700;   // ms
const uint32_t LOCKOUT  = 1500;  // ms ignore after a shake

float baseline = 1.0;
bool wasAbove = false;
int peaks = 0;
uint32_t winStart = 0, lockUntil = 0;
float maxDev = 0;

void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg); Wire.write(val);
  Wire.endTransmission();
}

bool readMag(float &mag) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)6) != 6) return false;
  int16_t rx = (Wire.read() << 8) | Wire.read();
  int16_t ry = (Wire.read() << 8) | Wire.read();
  int16_t rz = (Wire.read() << 8) | Wire.read();
  float ax = rx / 16384.0, ay = ry / 16384.0, az = rz / 16384.0;
  mag = sqrt(ax * ax + ay * ay + az * az);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  writeReg(0x6B, 0x00);   // wake
  writeReg(0x1A, 0x03);   // low-pass filter ~44 Hz, less noise
  writeReg(0x1C, 0x00);   // +/-2g
  delay(100);

  Serial.println("Keep the board still for 2 seconds...");
  float sum = 0; int n = 0; float m;
  for (int i = 0; i < 200; i++) {
    if (readMag(m)) { sum += m; n++; }
    delay(10);
  }
  if (n) baseline = sum / n;
  Serial.printf("Baseline = %.3f g. Shake it now.\n", baseline);
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last < 10) return;   // 100 Hz
  last = millis();

  float mag;
  if (!readMag(mag)) { Serial.println("Read failed"); return; }
  float dev = fabs(mag - baseline);
  if (dev > maxDev) maxDev = dev;

  bool above = dev > THRESH;
  if (above && !wasAbove) {
    uint32_t now = millis();
    if (peaks == 0 || now - winStart > WINDOW) { peaks = 0; winStart = now; }
    peaks++;
    if (peaks >= PEAKS && now > lockUntil) {
      Serial.printf("SHAKE  (max deviation %.2f g)\n", maxDev);
      lockUntil = now + LOCKOUT;
      peaks = 0;
      maxDev = 0;
    }
  }
  wasAbove = above;
}