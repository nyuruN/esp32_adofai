#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#ifdef __EMSCRIPTEN__
#include <LGFX_AUTODETECT.hpp>
inline LGFX lcd(320, 240);
#else
#include "LGFX_ESP32_S3_LCD_2.hpp"
inline LGFX lcd;
#endif

inline LGFX_Sprite _sprites[2];

inline static void setup_display(void)
{
  lcd.init();

  lcd.startWrite();
  lcd.setColorDepth(8);
  if (lcd.width() < lcd.height())
    lcd.setRotation(lcd.getRotation() ^ 1);

  auto lcd_width = lcd.width();
  auto lcd_height = lcd.height();

  for (std::uint32_t i = 0; i < 2; ++i)
  {
    _sprites[i].setTextSize(2);
    _sprites[i].setColorDepth(8);
  }

  bool fail = false;
  for (std::uint32_t i = 0; !fail && i < 2; ++i)
  {
    fail = !_sprites[i].createSprite(lcd_width, lcd_height);
  }

#if defined(ESP_PLATFORM)
  if (fail)
  {
    fail = false;
    for (std::uint32_t i = 0; !fail && i < 2; ++i)
    {
      _sprites[i].setPsram(true);
      fail = !_sprites[i].createSprite(lcd_width, lcd_height);
    }

    if (fail)
    {
      lcd.print("createSprite fail...");
      lgfx::delay(3000);
    }
  }
#endif
}

// Perform partial refresh
inline static void diffdraw(LGFX_Sprite *sp0, LGFX_Sprite *sp1)
{
  union
  {
    std::uint32_t *s32;
    std::uint8_t *s;
  };
  union
  {
    std::uint32_t *p32;
    std::uint8_t *p;
  };
  s32 = (std::uint32_t *)sp0->getBuffer();
  p32 = (std::uint32_t *)sp1->getBuffer();

  auto width = sp0->width();
  auto height = sp0->height();

  auto w32 = (width + 3) >> 2;
  std::int32_t y = 0;
  do
  {
    std::int32_t x32 = 0;
    do
    {
      while (s32[x32] == p32[x32] && ++x32 < w32)
        ;
      if (x32 == w32)
        break;

      std::int32_t xs = x32 << 2;
      while (s[xs] == p[xs])
        ++xs;

      while (++x32 < w32 && s32[x32] != p32[x32])
        ;

      std::int32_t xe = (x32 << 2) - 1;
      if (xe >= width)
        xe = width - 1;
      while (s[xe] == p[xe])
        --xe;

      lcd.pushImage(xs, y, xe - xs + 1, 1, &s[xs]);
    } while (x32 < w32);
    s32 += w32;
    p32 += w32;
  } while (++y < height);
  lcd.display();
}