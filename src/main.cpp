
#include <LovyanGFX.hpp>
#include "LGFX_ESP32_S3_LCD_2.hpp"

// Covenient Rust-like type definitions
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

static LGFX lcd;
static LGFX_Sprite _sprites[2];

static constexpr std::uint32_t SHIFTSIZE = 8;

static std::uint32_t _fps = 0;
static std::uint32_t sec, psec;
static std::uint32_t fps = 0, frame_count = 0;

static std::uint32_t _width;
static std::uint32_t _height;

bool _is_running;
std::uint32_t _draw_count;
std::uint32_t _loop_count;

static void diffDraw(LGFX_Sprite* sp0, LGFX_Sprite* sp1)
{
  union
  {
    std::uint32_t* s32;
    std::uint8_t* s;
  };
  union
  {
    std::uint32_t* p32;
    std::uint8_t* p;
  };
  s32 = (std::uint32_t*)sp0->getBuffer();
  p32 = (std::uint32_t*)sp1->getBuffer();

  auto width  = sp0->width();
  auto height = sp0->height();

  auto w32 = (width+3) >> 2;
  std::int32_t y = 0;
  do
  {
    std::int32_t x32 = 0;
    do
    {
      while (s32[x32] == p32[x32] && ++x32 < w32);
      if (x32 == w32) break;

      std::int32_t xs = x32 << 2;
      while (s[xs] == p[xs]) ++xs;

      while (++x32 < w32 && s32[x32] != p32[x32]);

      std::int32_t xe = (x32 << 2) - 1;
      if (xe >= width) xe = width - 1;
      while (s[xe] == p[xe]) --xe;

      lcd.pushImage(xs, y, xe - xs + 1, 1, &s[xs]);
    } while (x32 < w32);
    s32 += w32;
    p32 += w32;
  } while (++y < height);
  lcd.display();
}

static void drawfunc(void)
{
  LGFX_Sprite *sprite;

  auto width  = _sprites[0].width();
  auto height = _sprites[0].height();

  std::size_t flip = _draw_count & 1;

  sprite = &(_sprites[flip]);
  sprite->clear();

  for (int32_t i = 8; i < width; i += 16) {
    sprite->drawFastVLine(i, 0, height, 0x1F);
  }
  for (int32_t i = 8; i < height; i += 16) {
    sprite->drawFastHLine(0, i, width, 0x1F);
  }

  sprite->setCursor(1,1);
  sprite->setTextColor(TFT_BLACK);
  sprite->printf("fps:%d", (int)_fps);
  sprite->setCursor(0,0);
  sprite->setTextColor(TFT_WHITE);
  sprite->printf("fps:%d", (int)_fps);

  diffDraw(&_sprites[flip], &_sprites[!flip]);
  ++_draw_count;
}

static void mainfunc(void)
{
  sec = lgfx::millis() / 1000;
  if (psec != sec) {
    psec = sec;
    fps = frame_count;
    frame_count = 0;
    vTaskDelay(1);
  }

  frame_count++;
  _loop_count++;
  _fps = fps;
}

void setup(void)
{
  lcd.begin();
  lcd.startWrite();
  lcd.setColorDepth(8);
  if (lcd.width() < lcd.height()) lcd.setRotation(lcd.getRotation() ^ 1);

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
      fail = false;
      if (lcd_width > 320) lcd_width = 320;
      if (lcd_height > 240) lcd_height = 240;

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
  }

  _width = lcd_width << SHIFTSIZE;
  _height = lcd_height << SHIFTSIZE;

  _is_running = true;
  _draw_count = 0;
  _loop_count = 0;
}

void loop(void)
{
  mainfunc();
  drawfunc();
}

extern "C" void app_main()
{
  setup();
  for (;;) loop();
}