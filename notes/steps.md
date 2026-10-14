Stage 0: Confirm your board (2 minutes)

Your port came up as /dev/ttyACM0, which usually means a native-USB chip (S2, S3 or C3), not a classic ESP32. [Likely] Read the chip name on the metal shield. If it’s not a classic ESP32, the pins below (SDA 21, SCL 22) and the “ESP32 Dev Module” setting may be wrong, so tell me the chip and I’ll correct them.

Stage 1: Toolchain check (blink)

Upload File → Examples → 01.Basics → Blink, using the board’s real LED pin (LED_BUILTIN may not be defined on every ESP32 board). [Likely] If this uploads, your port, permissions and board package all work.

Stage 2: Wire with the power off  
OLED and MPU6050 both go to 3V3 (not 5V), GND, SDA and SCL, sharing the two I2C lines. [Certain] Many MPU6050 breakout boards tolerate 5V on VCC, but their logic pins are 3.3V, and the ESP32’s pins are not 5V tolerant.  
Button A to GPIO 25 and GND, button B to GPIO 33 and GND. No external resistors needed with INPUT_PULLUP. [Certain]  
Use a common ground for everything, and keep jumper wires short (under ~15 cm) for I2C at 400 kHz. [Likely]  
Solder headers if the breadboard connection is loose, since breadboard contacts are the most common source of “random” I2C failures. [Likely]  
Stage 3: I2C scanner

Upload this and open Serial Monitor at 115200 baud:

cpp  
#include <Wire.h>  
void setup() {  
Serial.begin(115200);  
Wire.begin(21, 22); // SDA, SCL — change for your board  
}  
void loop() {  
for (byte a = 1; a < 127; a++) {  
Wire.beginTransmission(a);  
if (Wire.endTransmission() == 0) { Serial.print("Found 0x"); Serial.println(a, HEX); }  
}  
Serial.println("--"); delay(2000);  
}

Expected: 0x3C (OLED, sometimes 0x3D) and 0x68 (MPU, 0x69 if its AD0 pin is high). [Certain]

Nothing found: check power and SDA/SCL swap.  
Only one device found: look at that device’s wiring.  
Address 0x70-ish or something unexpected: your module may be a different chip.  
Stage 4: One device at a time  
OLED: run File → Examples → Adafruit SSD1306 → ssd1306_128x64_i2c. Confirm the screen size (128x64 or 128x32) and the driver chip. If the image is shifted or has noise on the edge, it’s an SH1106, so use Adafruit SH110X. [Likely]  
MPU6050: run File → Examples → Adafruit MPU6050 → basic_readings. Check the readings: lying flat, Z should read about 9.8 m/s² and X and Y about 0. Shake it and watch the values. Write down the peak magnitudes of a hard shake, since you’ll set your threshold from them. [Certain]  
Buttons: print the state of GPIO 25 and 33 to Serial. Confirm they read HIGH idle and LOW pressed. Watch for bounce.  
Stage 5: RoboEyes alone

Open File → Examples → FluxGarage RoboEyes and run an example unchanged. Then try the moods (HAPPY, ANGRY, TIRED), anim_laugh() and anim_confused().

Stage 6: Integration, one feature at a time

Order: eyes + button A (pet) → shake detection (dizzy) → button B (sleep) → time display → WiFi NTP. Commit to git after each working step. [Likely] That way a bad change is easy to revert.

Git hygiene  
Create a secrets.h for the WiFi name and password, and add it to .gitignore, so you don’t push your credentials. [Certain]  
Commit a secrets.example.h with placeholder values.  
Known risks  
WiFi + deep sleep + OLED: every boot from deep sleep loses the time unless you re-sync, so decide on the RTC question before you design the time feature. [Certain]  
Power: USB power is fine for testing. For portable use you’ll need a LiPo and charger board, and OLED + WiFi bursts can brown out a weak supply. [Likely]

Run Stage 3 and send me the scanner output plus the chip name from Stage 0. That tells me whether your pin assumptions hold and which OLED driver to use.