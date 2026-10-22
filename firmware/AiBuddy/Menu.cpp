#include "Menu.h"
#include "Config.h"
#include "Ui.h"

void Menu::setEntries(const MenuEntry* entries, uint8_t count) {
  entries_ = entries; count_ = count; sel_ = top_ = 0;
}

void Menu::move(int dir) {
  if (count_ == 0) return;
  sel_ = (uint8_t)((sel_ + count_ + (dir < 0 ? -1 : 1)) % count_);
  if (sel_ < top_) top_ = sel_;                                             // scrolled above the window
  else if (sel_ >= top_ + cfg::MENU_VISIBLE_ROWS) top_ = (uint8_t)(sel_ - cfg::MENU_VISIBLE_ROWS + 1);
}

void Menu::draw(Adafruit_GFX& g) const {
  drawTitle(g);
  const uint8_t shown = (uint8_t)(count_ < cfg::MENU_VISIBLE_ROWS ? count_ : cfg::MENU_VISIBLE_ROWS);
  for (uint8_t i = 0; i < shown; i++) drawRow(g, (uint8_t)(top_ + i), cfg::MENU_TOP + i * cfg::MENU_ROW_H);
  if (count_ > cfg::MENU_VISIBLE_ROWS) drawScrollbar(g);
}

void Menu::drawTitle(Adafruit_GFX& g) const {
  if (cfg::MENU_POLISHED) {
    ui::textCentered(g, 1, cfg::MENU_TITLE, cfg::C_ON, true);
    g.drawFastHLine(4, cfg::MENU_TITLE_H - 1, cfg::SCREEN_W - 8, cfg::C_ON);
  } else {
    g.drawRect(0, 0, cfg::SCREEN_W, cfg::MENU_TITLE_H, cfg::C_ON);
    ui::textCentered(g, (cfg::MENU_TITLE_H - ui::CHAR_H) / 2, cfg::MENU_TITLE, cfg::C_ON);
  }
}

void Menu::drawRow(Adafruit_GFX& g, uint8_t index, int y) const {
  const MenuEntry& e = entries_[index];
  const bool sel = index == sel_;
  const int textY = y + (cfg::MENU_ROW_H - ui::CHAR_H) / 2;
  const int iconY = y + (cfg::MENU_ROW_H - icons::SIZE) / 2;

  uint16_t fg = cfg::C_ON;
  if (sel && !cfg::MENU_POLISHED) {                      // classic: white bar, dark text
    g.fillRect(cfg::MENU_BAR_X, y, cfg::MENU_BAR_W, cfg::MENU_ROW_H, cfg::C_ON);
    fg = cfg::C_OFF;
  }
  if (sel) ui::text(g, cfg::MENU_CHEVRON_X, textY, ">", cfg::C_ON);
  icons::draw(g, e.icon, cfg::MENU_ICON_X, iconY, fg);
  ui::text(g, cfg::MENU_TEXT_X, textY, e.label, fg, cfg::MENU_POLISHED && sel);
}

void Menu::drawScrollbar(Adafruit_GFX& g) const {
  const int trackH = cfg::MENU_VISIBLE_ROWS * cfg::MENU_ROW_H;
  const int x = cfg::SCREEN_W - 2;
  int thumbH = trackH * cfg::MENU_VISIBLE_ROWS / count_;
  if (thumbH < 4) thumbH = 4;
  const int span = count_ - cfg::MENU_VISIBLE_ROWS;      // > 0 here
  const int thumbY = cfg::MENU_TOP + (trackH - thumbH) * top_ / span;
  g.drawFastVLine(x, cfg::MENU_TOP, trackH, cfg::C_ON);
  g.fillRect(x - 1, thumbY, 3, thumbH, cfg::C_ON);
}
