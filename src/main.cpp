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

// Global objects
static LGFX lcd;
static LGFX_Sprite _sprites[2];

// Auxiliary variables
static std::uint32_t _fps = 0;
static std::uint32_t sec, psec;
static std::uint32_t fps = 0, frame_count = 0;
bool _is_running;
std::uint32_t _draw_count;
std::uint32_t _loop_count;

// Fixed point math variables
static constexpr std::uint32_t SHIFTSIZE = 8;
static std::uint32_t _width;
static std::uint32_t _height;

// Perform partial refresh
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

  /*
  if (flip) {
    sprite->fillRect(0, 0, width, height, sprite->color332(255, 0, 0));
  } else {
    sprite->fillRect(0, 0, width, height, sprite->color332(0, 0, 0));
  }
  */
  
  sprite->fillRect(0, 0, width, height, sprite->color332(255, 0, 0));

  sprite->fillCircle(width / 2, height / 2, (height / 2 - 20) * (_loop_count % 256) / 256, sprite->color332(0, 0, 0));
  sprite->fillCircle(width / 2, height / 2, ((height - 20) / 2 - 20) * (_loop_count % 256) / 256, sprite->color332(255, 255, 255));

  /*
  for (int32_t i = 8; i < width; i += 16) {
    sprite->drawFastVLine(i, 0, height, 0x1F);
  }
  for (int32_t i = 8; i < height; i += 16) {
    sprite->drawFastHLine(0, i, width, 0x1F);
  }
  */

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

void setup_display(void)
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

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_flash.h"

#include "esp_log.h"
#include "esp_psram.h"
#include "esp_spi_flash.h"

#include "esp_flash_spi_init.h"

void print_memory_info(void) {
    // Get total and free sizes for different memory types
    multi_heap_info_t info;

    // 1. Internal SRAM (IRAM/DRAM) - MALLOC_CAP_INTERNAL
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    ESP_LOGI("Main", "\n--- Internal SRAM (IRAM/DRAM) ---\n");
    ESP_LOGI("Main", "Total: %d KB\n", (info.total_free_bytes + info.total_allocated_bytes) / 1024);
    ESP_LOGI("Main", "Free: %d KB\n", info.total_free_bytes / 1024);
    ESP_LOGI("Main", "Largest Free Block: %d KB\n", info.largest_free_block / 1024);

//size_t psram_size = esp_psram_get_size();
//ESP_LOGI("Main", "PSRAM size: %d bytes\n", (int)psram_size);

    // 2. External PSRAM (SPIRAM) - MALLOC_CAP_SPIRAM
    if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > 0) {
        heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
        ESP_LOGI("Main", "\n--- External PSRAM (SPIRAM) ---\n");
        ESP_LOGI("Main", "Total: %d KB\n", (info.total_free_bytes + info.total_allocated_bytes) / 1024);
        ESP_LOGI("Main", "Free: %d KB\n", info.total_free_bytes / 1024);
        ESP_LOGI("Main", "Largest Free Block: %d KB\n", info.largest_free_block / 1024);
    } else {
        ESP_LOGI("Main", "\n--- External PSRAM (SPIRAM) ---\n");
        ESP_LOGI("Main", "PSRAM is not enabled or not detected.\n");
    }

    // 3. Flash Size (Storage, not RAM)
    ESP_LOGI("Main", "\n--- Flash (Storage) ---\n");

    uint32_t flash_size;
    auto res = esp_flash_get_size(NULL, &flash_size);

    if (res == ESP_OK) {
      ESP_LOGI("Main", "Total Flash Size: %d MB\n", (int)flash_size / (1024 * 1024));
    } else {
      ESP_LOGI("Main", "Failed to get flash size.\n");
    }

    // Optional: Print a summary from the main heap
    ESP_LOGI("Main", "\n--- Summary ---\n");
    ESP_LOGI("Main", "Total Free Heap (all memory): %d KB\n", (int)esp_get_free_heap_size() / 1024);
    ESP_LOGI("Main", "Minimum Free Heap Ever: %d KB\n", (int)esp_get_minimum_free_heap_size() / 1024);
}

extern "C" void app_main()
{
  ESP_LOGI("Main", "Hello, ESP32!");
  setup_display();
  print_memory_info();
  for (;;) loop();
}