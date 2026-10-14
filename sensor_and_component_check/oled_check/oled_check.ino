// OLED-ONLY TEST: ESP32 DevKit V1, I2C on GPIO 21/22
// Phases: 1 scan -> 2 border+text -> 3 static eyes -> 4 RoboEyes animation

#include <Wire.h>
#include <Adafruit_GFX.h>

// ---- EDIT THESE IF NEEDED ----
#define USE_SH1106     0     // 0 = SSD1306 (default), 1 = SH1106
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64    // set to 32 if your panel is 128x32
// ------------------------------

#define SDA_PIN 21
#define SCL_PIN 22
#define OLED_RESET -1
#define C_ON 1               // white pixel for both drivers

#if USE_SH1106
  #include <Adafruit_SH110X.h>
  Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  #define EYES_DRIVER Adafruit_SH1106G
#else
  #include <Adafruit_SSD1306.h>
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  #define EYES_DRIVER Adafruit_SSD1306
#endif

#include <FluxGarage_RoboEyes.h>
RoboEyes<EYES_DRIVER> roboEyes(display);

uint8_t oledAddr = 0;

void halt(const char* msg) {
  Serial.println(msg);
  while (true) delay(1000);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Wire.begin(SDA_PIN, SCL_PIN);

  // ---- PHASE 1: I2C scan ----
  Serial.println("PHASE 1: I2C scan");
  uint8_t found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      if (a < 16) Serial.print("0");
      Serial.println(a, HEX);
      found++;
      if (oledAddr == 0 && (a == 0x3C || a == 0x3D)) oledAddr = a;
    }
  }
  Serial.print("Devices found: ");
  Serial.println(found);
  if (found == 0)  halt("FAIL: no I2C device found. Check wiring/power.");
  if (oledAddr == 0) halt("FAIL: devices found but none at 0x3C/0x3D.");
  Serial.print("Using OLED address 0x");
  Serial.println(oledAddr, HEX);

  // ---- PHASE 2: init + border + text ----
#if USE_SH1106
  delay(250);
  if (!display.begin(oledAddr, true)) halt("FAIL: display.begin failed");
#else
  if (!display.begin(SSD1306_SWITCHCAPVCC, oledAddr)) halt("FAIL: display.begin failed");
#endif
  Serial.println("OLED OK");

  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C_ON);
  display.setTextSize(1);
  display.setTextColor(C_ON);
  display.setCursor(6, 6);
  display.print(SCREEN_WIDTH); display.print("x"); display.print(SCREEN_HEIGHT);
  display.setCursor(6, 16);
  display.print("Addr 0x"); display.print(oledAddr, HEX);
  display.display();
  Serial.println("PHASE 2: border + text drawn (hold 4s)");
  delay(4000);

  // ---- PHASE 3: two static rounded-rectangle eyes ----
  int eh = (SCREEN_HEIGHT >= 64) ? 36 : 22;
  int ey = (SCREEN_HEIGHT - eh) / 2;
  display.clearDisplay();
  display.fillRoundRect(22, ey, 36, eh, 8, C_ON);
  display.fillRoundRect(70, ey, 36, eh, 8, C_ON);
  display.display();
  Serial.println("PHASE 3: static eyes drawn (hold 4s)");
  delay(4000);

  // ---- PHASE 4: RoboEyes animation (runs forever) ----
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 60);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);
  Serial.println("PHASE 4: RoboEyes running");
}

void loop() {
  roboEyes.update();   // no delay() here
}