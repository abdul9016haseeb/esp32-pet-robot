

## rocket shooter's componenets Hardware

| Component                 | Quantity |
| ------------------------- | -------- |
| ESP32 DevKit              | 1        |
| SSD1306 OLED 128x64 (I2C) | 1        |
| MPU6050 Accelerometer     | 1        |
| Breadboard                | 1        |
| Jumper wires              | —        |

## Wiring

Both OLED and MPU6050 share the same I2C bus.

| Pin                  | ESP32 GPIO |
| -------------------- | ---------- |
| SDA (OLED + MPU6050) | GPIO 21    |
| SCL (OLED + MPU6050) | GPIO 22    |
| VCC (both)           | 3.3V       |
| GND (both)           | GND        |
| MPU6050 AD0          | GND        |



# dino game's files

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
