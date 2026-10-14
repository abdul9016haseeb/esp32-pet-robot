#include <Wire.h>

#define SDA_PIN 21
#define SCL_PIN 22
const uint8_t MPU_ADDR = 0x68;

void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);  // slow and safe for first test

  // 1) I2C scan
  Serial.println("Scanning I2C...");
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Found device at 0x%02X\n", a);
      found++;
    }
  }
  if (!found) Serial.println("No I2C devices found. Check wiring.");

  // 2) Wake the MPU6050 (it starts in sleep mode)
  writeReg(0x6B, 0x00);
  delay(100);
  writeReg(0x1C, 0x00);  // accel range +/-2g -> 16384 LSB per g

  // 3) WHO_AM_I
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x75);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, (uint8_t)1);
  if (Wire.available()) Serial.printf("WHO_AM_I = 0x%02X (genuine MPU6050 = 0x68)\n", Wire.read());
  else Serial.println("No WHO_AM_I response");
}

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);  // ACCEL_XOUT_H
  Wire.endTransmission(false);
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)6) == 6) {
    int16_t rx = (Wire.read() << 8) | Wire.read();
    int16_t ry = (Wire.read() << 8) | Wire.read();
    int16_t rz = (Wire.read() << 8) | Wire.read();
    float ax = rx / 16384.0, ay = ry / 16384.0, az = rz / 16384.0;
    float mag = sqrt(ax * ax + ay * ay + az * az);
    Serial.printf("ax=%.2f ay=%.2f az=%.2f |a|=%.2f g\n", ax, ay, az, mag);
  } else {
    Serial.println("Read failed");
  }
  delay(200);
}