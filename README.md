# ESP32 Pet Robot ("AI Buddy")

A pocket desk pet on an ESP32. Two animated eyes react to physics (tilt, shake, free fall, landing) and a
small button-driven "operating system" hosts apps: clock, timer, stopwatch, and later weather and games.

> Status: work in progress. See [Verification status](#verification-status) for what has actually been run on hardware.

## Hardware

| Part | Notes |
|---|---|
| ESP32 DevKit V1 (classic ESP32) | Arduino board: *ESP32 Dev Module* |
| SSD1306 OLED 128x64, I2C | address `0x3C` |
| MPU6050 IMU | address `0x68` (AD0 to GND) |
| 6 push buttons | wired to GND, `INPUT_PULLUP`, no resistors |
| Active buzzer (optional) | GPIO 5, off by default (`cfg::BUZZER_ENABLED`) |

| Signal | GPIO | Signal | GPIO |
|---|---|---|---|
| SDA | 21 | UP | 19 |
| SCL | 22 | DOWN | 18 |
| LEFT | 17 | RIGHT | 16 |
| CENTER | 4 | EXIT | 23 |

Power OLED and MPU6050 from 3V3, not 5V. All pins are defined in `firmware/AiBuddy/Config.h`.

## Quick start

1. Install the ESP32 board package and the libraries **Adafruit SSD1306** and **Adafruit GFX**.
2. Open `firmware/AiBuddy/AiBuddy.ino` in the Arduino IDE (the folder name must match the `.ino` name).
3. Select *ESP32 Dev Module*, upload, keep the board still during "Calibrating...".
4. Serial monitor at 115200 baud should print `[buddy] ready`.

Before first use on new hardware, run the sketches in `firmware/bench/` in this order:
`i2c_scanner` -> `oled_check` -> `mpu_sensor_check` -> `oled_and_mpu` -> `button_check`.
(`oled_check` additionally needs the FluxGarage RoboEyes library.)

## Controls

| Screen | Input |
|---|---|
| Eyes | CENTER opens the apps menu |
| Menu | UP/DOWN move, CENTER opens, EXIT returns to the eyes |
| App | EXIT returns to the menu; other buttons belong to the app |
| Clock app | LEFT/RIGHT change page (Clock, Timer, Stopwatch); see `docs/code-guide-addendum.txt` |

## Repository layout

```
firmware/
  AiBuddy/        the firmware (one flat Arduino sketch folder)
  bench/          single-purpose hardware bring-up sketches, each with its test_result.md
docs/
  architecture.md system layers (drivers, core, services, shell, apps, personality)
  code-guide.txt  file-by-file reference and tuning table
  code-guide-addendum.txt  menu, buttons, clock, timer alert, buzzer specs and bench checklists
  plans/  specs/  notes/  reference/
```

## Verification status

Update this table whenever a bench checklist is run. Only mark "bench" after it ran on the board.

| Area | Status | Evidence |
|---|---|---|
| OLED (SSD1306, 0x3C) | Bench PASS | `firmware/bench/oled_check/test_result.md` |
| MPU6050 (rest, shake) | Bench PASS; slow tilt and tap unverified | `firmware/bench/mpu_sensor_check/test_result.md` |
| Eyes + shake / free-fall behavior | Run on board, thresholds still being tuned | serial logs |
| Buttons + apps menu | Record the checklist result (addendum 12.6) | TODO |
| Clock / timer / stopwatch / timer alert | Run on board (2026-10-22, owner-reported); write up the checklist items | TODO: add test_result.md |
| Buzzer | Not wired, disabled | n/a |
| WiFi, weather, games, SD music | Not started | n/a |

## Contributing / workflow

- One milestone per branch (`feat/<name>`), each with a bench checklist and a result file.
- No `delay()` in firmware except in `fatal()`; every tunable lives in `Config.h`.
- New event types are added only at the end of `enum EventType`.
- Never commit WiFi credentials: use `secrets.h` (git-ignored) plus a committed `secrets.example.h`.

## License

TBD (choose one before making the repository public)..
