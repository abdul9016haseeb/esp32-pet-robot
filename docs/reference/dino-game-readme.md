# 🌟 Google Dino Game — ESP32 + OLED
**EmbedForge Project**

A fully functional Google Chrome Dino Game built on ESP32 with a 128x64 OLED display. Button controlled jump, pixel accurate sprites, scrolling ground, score system, speed ramp, bye bye animation on death, and jump/die sounds played via Python serial on laptop.

## Demo Video
Youtube vido link: https://youtube.com/shorts/U2lNe--PHUU

---

## Components

| Component | Specification |
|---|---|
| Microcontroller | ESP32 Dev Module |
| Display | SSD1306 OLED 128x64 (I2C) |
| Button | Push button (momentary) |
| Cable | USB (flashing + serial sound) |

---

## Circuit Diagram

```
                        ESP32 Dev Board
                   ┌──────────────────────┐
                   │                      │
    OLED VCC ──────┤ 3.3V                 │
    OLED GND ──────┤ GND                  │
    OLED SDA ──────┤ GPIO 21              │
    OLED SCL ──────┤ GPIO 22              │
                   │                      │
    BUTTON ────────┤ GPIO 4               │
    BUTTON ────────┤ GND                  │
                   │                      │
    USB TO LAPTOP──┤ USB                  │
                   └──────────────────────┘


    OLED SSD1306 (128x64)
    ┌─────────────┐
    │  VCC        │──── 3.3V
    │  GND        │──── GND
    │  SDA        │──── GPIO 21
    │  SCL        │──── GPIO 22
    └─────────────┘

    PUSH BUTTON
    ┌─────┐
    │     │ Leg 1 ──── GPIO 4
    │     │ Leg 2 ──── GND
    └─────┘
    (INPUT_PULLUP — no resistor needed)
```

---

## Software

| Software | Version |
|---|---|
| Arduino IDE | 2.x |
| Espressif ESP32 Board Package | Latest |
| Adafruit SSD1306 Library | Latest |
| Adafruit GFX Library | Latest |
| Python 3 | System |
| pyserial | pip3 install |
| pygame | pip3 install |

---

## Library Install — Arduino IDE

```
Tools → Manage Libraries → Search and Install:
  - Adafruit SSD1306
  - Adafruit GFX Library
```

---

## Python Install — Linux

```bash
pip3 install pyserial pygame --break-system-packages
```

---

## Project Structure

```
dino-game-esp32/
├── dino_pixel_accurate_esp32.ino    # ESP32 Arduino code
├── dino_sound.py                    # Python sound script
├── jump.wav                         # Jump sound effect
├── die.wav                          # Die sound effect
└── README.md                        # This file
```

---

## How It Works

### ESP32 Side
- Draws pixel accurate dino and cactus sprites using `drawBitmap()` with PROGMEM data
- Button on GPIO 4 triggers dino jump via edge detection
- Collision detection uses shrunk AABB hitbox for fair gameplay
- Score increments every 150ms — speed increases every 15 points up to max 9
- On death: bye bye animation plays, then GAME OVER screen waits for button to restart
- Sends `"JUMP"` and `"DIE"` over USB Serial at 115200 baud

### Python Side
- Listens on `/dev/ttyACM0` at 115200 baud
- On receiving `"JUMP"` → plays `jump.wav`
- On receiving `"DIE"` → plays `die.wav`

---

## Game Features

| Feature | Detail |
|---|---|
| Dino sprite | 16x16 pixel bitmap, 2 walk frames |
| Cactus sprite | 8x16 pixel bitmap |
| Ground | Scrolling pebble dots |
| Score | Live score + high score |
| Speed | Starts at 3, ramps to 9 |
| Jump sound | Via Python serial |
| Die sound | Via Python serial |
| Death animation | Sad face waving bye bye |
| Display rotation | `setRotation(2)` for inverted OLED |

---

## Run Instructions

```
1. Open dino_pixel_accurate_esp32.ino in Arduino IDE
2. Select board: ESP32 Dev Module
3. Select correct COM port
4. Upload to ESP32
5. Close Arduino IDE Serial Monitor
6. Open terminal in project folder
7. python3 dino_sound.py
8. Press button on ESP32 to start game
```

---

## Wiring Summary

| ESP32 GPIO | Connected To |
|---|---|
| 3.3V | OLED VCC |
| GND | OLED GND |
| GPIO 21 | OLED SDA |
| GPIO 22 | OLED SCL |
| GPIO 4 | Button leg 1 |
| GND | Button leg 2 |
| USB | Laptop (serial sound) |

---

## Notes

- GPIO 15 is a strapping pin on ESP32 — do not use for button
- OLED runs on 3.3V pin — safe, no current issues
- Serial.println for sound adds ~0.1ms delay — no gameplay impact
- `setRotation(2)` placed immediately after `display.begin()` in setup

---

## Channel

**EmbedForge** — Embedded Systems, Firmware, IoT, Linux, Electronics.
**Channel Link** - www.youtube.com/@EmbedForge-abd
