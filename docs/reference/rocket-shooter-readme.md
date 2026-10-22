# Rocket Shooter — ESP32 + OLED + MPU6050

A fully playable rocket shooter game on a 128x64 OLED display controlled by tilting the circuit using an accelerometer.

---

## Demo
> Watch on YouTube → https://youtube.com/shorts/gkutlVaawtE

---

## Hardware

| Component | Quantity |
|---|---|
| ESP32 DevKit | 1 |
| SSD1306 OLED 128x64 (I2C) | 1 |
| MPU6050 Accelerometer | 1 |
| Breadboard | 1 |
| Jumper wires | — |

---

## Wiring

Both OLED and MPU6050 share the same I2C bus.

| Pin                  | ESP32 GPIO |
| -------------------- | ---------- |
| SDA (OLED + MPU6050) | GPIO 21    |
| SCL (OLED + MPU6050) | GPIO 22    |
| VCC (both)           | 3.3V       |
| GND (both)           | GND        |
| MPU6050 AD0          | GND        |

---

## Libraries

Install via Arduino Library Manager:
- Adafruit SSD1306
- Adafruit GFX Library

---

## How to Run

**1. Upload Arduino sketch**
```
Open rocket_shooter_final.ino in Arduino IDE
Select board: ESP32 Dev Module
Upload
```

**2. Run sound engine on PC**
```bash
pip install pyserial pygame
cd sounds/
python ../sound_engine.py
```

Edit `PORT` in `sound_engine.py` to match your ESP32 COM port.

**3. Place sound files in `sounds/` folder**
```
sounds/
├── fire.wav
├── explode.wav
├── death.wav
└── start.wav
```

---

## Controls

| Tilt | Action |
|---|---|
| Left | Rocket moves left |
| Right | Rocket moves right |
| Forward | Rocket moves up |
| Backward | Rocket moves down |

---

## Game Rules

- Rocket fires bullets automatically
- Asteroids spawn from the top and fall down
- Bullet hits asteroid → explosion + score
- Asteroid hits rocket → game over
- Difficulty increases as score rises

---

## Project Structure

```
rocket-shooter-esp32/
├── rocket_shooter_final.ino
├── sound_engine.py
├── README.md
├── description.txt
└── sounds/
    ├── fire.wav
    ├── explode.wav
    ├── death.wav
    └── start.wav
```

---

## Author

EmbedForge — Embedded Systems Projects, 
Youtube Channel Link: www.youtube.com/@EmbedForge-abd
