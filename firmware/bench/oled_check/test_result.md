OLED test result: PASS (2026-10-14)

Setup

ESP32 DevKit V1 with a 4-pin I2C SSD1306 OLED, 128x64
Wiring: VCC to 3V3, GND to GND, SCL to GPIO 22, SDA to GPIO 21
MPU6050 was disconnected during the test
Libraries: Adafruit SSD1306, Adafruit GFX, FluxGarage RoboEyes
Sketch settings: USE_SH1106 0, 128x64, address 0x3C, serial at 115200

Results

I2C scan: one device at 0x3C
Serial: OLED OK, and all 4 phases ran
Phase 2: complete border and the correct “128x64, addr 0x3c” text
Phase 3: two clean static rounded-rectangle eyes
Phase 4: RoboEyes animation running, confirmed in your photos

Conclusion: The panel is an SSD1306 and works on GPIO 21/22 with the Adafruit driver. Keep this sketch as the known-good OLED baseline.

Not tested: MPU6050 together with the OLED on the same bus, buttons, and shake or touch features.