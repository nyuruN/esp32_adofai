#include <Arduino.h>

#include "rust_typedef.h"
#include "data.h"
#include "beatmap.h"
#include "lgfx.h"
#include "camera_events.h"

LGFX_Sprite _background;

// Auxiliary variables
static std::uint64_t pmillis = 0;
static std::uint32_t _fps = 0;
static std::uint32_t sec, psec;
static std::uint32_t fps = 0, frame_count = 0;
bool _is_running;
std::uint32_t _draw_count;
std::uint32_t _loop_count;

static void drawfunc(void)
{
  LGFX_Sprite* sprite;
  std::size_t flip = _draw_count & 1;
  sprite = &(_sprites[flip]);

  Beatmap::render(sprite);

  {
    // Debug info
    sprite->setTextColor(TFT_WHITE);
    sprite->setCursor(0,0);
    sprite->printf("fps:%d", (int)_fps);
    sprite->setCursor(0,20);
    sprite->printf("dir:%d", (int)Beatmap::current_angle);
    sprite->setCursor(0,40);
    sprite->printf("next:%d", (int)Beatmap::angle_data[current_floor]);
    sprite->setCursor(0,60);
    sprite->printf("prog:%d", (int)Beatmap::angle_progress);
    sprite->setCursor(0,80);
    sprite->printf("angle:%d", (int)Beatmap::angle_next);
    sprite->setCursor(0,100);
    sprite->printf("floor:%d", (int)Beatmap::current_floor);
    sprite->setCursor(0,120);
    sprite->printf("bpm:%d", (int)Beatmap::bpm);
    sprite->setCursor(0,140);
    sprite->printf("pE:%d", (int)Events::p_events);
    sprite->setCursor(0,160);
    sprite->printf("pCE:%d", (int)CameraEvents::count);
    sprite->setCursor(0,180);
    sprite->printf("zoom:%.2f", 1.0f / Beatmap::DrawData::_zoom);
    sprite->setCursor(0,200);
    sprite->printf("rot:%.2f", Beatmap::DrawData::_rotation);
    sprite->setCursor(0,220);
    sprite->printf("%.2f,%.2f", (Beatmap::DrawData::_camera_x - Beatmap::camera_x), (Beatmap::DrawData::_camera_y - Beatmap::camera_y));

    sprite->setCursor(185,0);
    sprite->printf("tiles: % 3d", Beatmap::TILE_BUF_SIZE);
    sprite->setCursor(185,20);
    sprite->printf("events:% 3d", Events::event_buf_size);
  }

  diffdraw(&_sprites[flip], &_sprites[!flip]);
  ++_draw_count;
}

static void mainfunc(void)
{
  float delta_time = (lgfx::millis() - pmillis) / 1000.0;
  pmillis = lgfx::millis();
  sec = lgfx::millis() / 1000;
  if (psec != sec) {
    psec = sec;
    fps = frame_count;
    frame_count = 0;
  }

  Beatmap::update(delta_time);

  // misc.
  frame_count++;
  _loop_count++;
  _fps = fps;
}

void setup_display(void)
{
  lcd.init();

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

  _is_running = true;
  _draw_count = 0;
  _loop_count = 0;
}

#include "esp_flash.h"
#include "esp_log.h"

void print_memory_info(void) {
    // Get total and free sizes for different memory types
    multi_heap_info_t info;

    // 1. Internal SRAM (IRAM/DRAM) - MALLOC_CAP_INTERNAL
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    ESP_LOGI("Main", "\n--- Internal SRAM (IRAM/DRAM) ---\n");
    ESP_LOGI("Main", "Total: %d KB\n", (info.total_free_bytes + info.total_allocated_bytes) / 1024);
    ESP_LOGI("Main", "Free: %d KB\n", info.total_free_bytes / 1024);
    ESP_LOGI("Main", "Largest Free Block: %d KB\n", info.largest_free_block / 1024);

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

#include <LittleFS.h>
#include <Arduino.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceLittleFS.h>

AudioGeneratorMP3 *mp3;
AudioFileSourceLittleFS *file;
AudioOutputI2S *out;

void setup(void) {
  setup_display();
  LittleFS.begin(true, "/littlefs", 10, "littlefs");

  _background.setTextSize(2);
  _background.setColorDepth(8);
  _background.setPsram(true);
  _background.createSprite(lcd.width(), lcd.height());
  _background.drawPngFile("/littlefs/bg.png", 0, 0, lcd.width(), lcd.height());

  file = new AudioFileSourceLittleFS("/audio.mp3");
  out = new AudioOutputI2S();
  mp3 = new AudioGeneratorMP3();

  out->SetPinout(12, 11, 14);
  out->SetGain(0.05);

  mp3->begin(file, out);

  Beatmap::clear();

  // Assign beatmap data
  Beatmap::set_beatmap_data(Data::angleData, Data::tileData, Data::tileCount);
  Beatmap::set_bpm(227);
  Beatmap::set_event_data(Data::eventData, Data::eventCount);

  Beatmap::begin();

  print_memory_info();
}

void loop(void) {
  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
    }
  }
  mainfunc();
  drawfunc();
}