// Icons.h - 12x12 one-bit menu icons, stored as 12 rows of 12 bits (leftmost pixel = bit 11).
// Written as binary literals so you can see and edit the picture directly: 1 = lit pixel.
// To add an icon: add an Id, a 12-row array, and a case in rows().
#pragma once
#include <Adafruit_GFX.h>

namespace icons {

constexpr int SIZE = 12;

enum class Id : uint8_t { None, Clock, Weather, Games, Exit };

static const uint16_t CLOCK[SIZE] PROGMEM = {
  0b000111111000,
  0b001000000100,
  0b010000100010,
  0b100000100001,
  0b100000100001,
  0b100000111001,
  0b100000000001,
  0b100000000001,
  0b010000000010,
  0b001000000100,
  0b000111111000,
  0b000000000000,
};

static const uint16_t WEATHER[SIZE] PROGMEM = {   // cloud
  0b000000000000,
  0b000000000000,
  0b000011100000,
  0b000100010000,
  0b001000001110,
  0b010000000001,
  0b010000000001,
  0b010000000001,
  0b001111111110,
  0b000000000000,
  0b000000000000,
  0b000000000000,
};

static const uint16_t GAMES[SIZE] PROGMEM = {     // game pad
  0b000000000000,
  0b000000000000,
  0b000000000000,
  0b011111111110,
  0b100100000001,
  0b101110001001,
  0b100100000101,
  0b100000000001,
  0b011110011110,
  0b000000000000,
  0b000000000000,
  0b000000000000,
};

static const uint16_t EXIT[SIZE] PROGMEM = {      // bracket + arrow pointing out
  0b000000000000,
  0b111111000000,
  0b100000000000,
  0b100000000100,
  0b100000000110,
  0b100111111111,
  0b100000000110,
  0b100000000100,
  0b100000000000,
  0b111111000000,
  0b000000000000,
  0b000000000000,
};

inline const uint16_t* rows(Id id) {
  switch (id) {
    case Id::Clock:   return CLOCK;
    case Id::Weather: return WEATHER;
    case Id::Games:   return GAMES;
    case Id::Exit:    return EXIT;
    default:          return nullptr;
  }
}

// Draws the icon with its top-left corner at (x, y); only lit pixels are drawn (transparent background).
inline void draw(Adafruit_GFX& g, Id id, int x, int y, uint16_t color) {
  const uint16_t* r = rows(id);
  if (!r) return;
  for (int row = 0; row < SIZE; row++) {
    const uint16_t bits = pgm_read_word(&r[row]);
    for (int col = 0; col < SIZE; col++) {
      if (bits & (1u << (SIZE - 1 - col))) g.drawPixel(x + col, y + row, color);
    }
  }
}

}  // namespace icons
