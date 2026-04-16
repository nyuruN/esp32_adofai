
#include "events.h"
#include "rust_typedef.h"
#include "beatmap.h"
#include "active_events.h"
#include "lgfx.h"
#include "data.h"
#include "app.h"

#ifdef __EMSCRIPTEN__
// If you write this, you can use drawBmpFile / drawJpgFile / drawPngFile
// #include <stdio.h>

// If you write this, you can use drawBmpUrl / drawJpgUrl / drawPngUrl ( for Windows )
// #include <windows.h>
// #include <winhttp.h>
// #pragma comment (lib, "winhttp.lib")
#include <emscripten.h>
#include <emscripten/bind.h>
extern "C"
{
  EMSCRIPTEN_KEEPALIVE
  void clear(bool erase_data) { BeatmapPlayer::clear(erase_data); }
  EMSCRIPTEN_KEEPALIVE
  void set_beatmap_data(u8 *angle_buf, u8 *tile_buf, u32 length) { BeatmapPlayer::set_beatmap_data(angle_buf, tile_buf, length); }
  EMSCRIPTEN_KEEPALIVE
  void set_bpm(float v) { BeatmapPlayer::set_bpm(v); }
  EMSCRIPTEN_KEEPALIVE
  void set_event_data(u8 *event_buf, u32 length) { BeatmapPlayer::set_event_data(event_buf, length); }
  EMSCRIPTEN_KEEPALIVE
  void play() { BeatmapPlayer::begin(); }
  EMSCRIPTEN_KEEPALIVE
  void hit() { BeatmapPlayer::hit(); }
};
#endif

App app;

// Auxiliary variables
u64 pmillis = 0;
u32 sec, psec;
u32 fps = 0, frame_count = 0;
u32 draw_count = 0;

LGFX_Sprite _background;

void drawfunc(void)
{
  LGFX_Sprite *sprite;
  std::size_t flip = draw_count & 1;
  sprite = &(_sprites[flip]);

  _background.pushSprite(sprite, 0, 0);

  app.render(sprite);

  {
    // Debug info
    sprite->setTextColor(TFT_WHITE);
    sprite->setCursor(0, 0);
    sprite->printf("fps:%d", (int)fps);
    sprite->setCursor(0, 20);
    sprite->printf("dir:%d", (int)BeatmapPlayer::current_angle);
    sprite->setCursor(0, 40);
    sprite->printf("next:%d", (int)BeatmapPlayer::angleData[current_floor]);
    sprite->setCursor(0, 60);
    sprite->printf("prog:%d", (int)BeatmapPlayer::angle_progress);
    sprite->setCursor(0, 80);
    sprite->printf("angle:%d", (int)BeatmapPlayer::angle_next);
    sprite->setCursor(0, 100);
    sprite->printf("floor:%d", (int)BeatmapPlayer::current_floor);
    sprite->setCursor(0, 120);
    sprite->printf("bpm:%.1f", BeatmapPlayer::bpm);
    sprite->setCursor(0, 140);
    sprite->printf("pE:%d", (int)Events::p_events);
    sprite->setCursor(0, 160);
    sprite->printf("pCE:%d", (int)ActiveEvents::count);
    sprite->setCursor(0, 180);
    sprite->printf("zoom:%.2f", 1.0f / BeatmapPlayer::DrawData::_zoom);
    sprite->setCursor(0, 200);
    sprite->printf("rot:%.2f", BeatmapPlayer::DrawData::_rotation);
    sprite->setCursor(0, 220);
    sprite->printf("%.2f,%.2f", (BeatmapPlayer::DrawData::_camera_x - BeatmapPlayer::camera_x), (BeatmapPlayer::DrawData::_camera_y - BeatmapPlayer::camera_y));

    sprite->setCursor(185, 0);
    sprite->printf("tiles: % 3d", BeatmapPlayer::tileCount);
    sprite->setCursor(185, 20);
    sprite->printf("events:% 3d", Events::event_buf_size);
  }

  diffdraw(&_sprites[flip], &_sprites[!flip]);
  draw_count++;
}

void mainfunc(void)
{
  float delta_time = (lgfx::millis() - pmillis) / 1000.0;
  pmillis = lgfx::millis();
  sec = lgfx::millis() / 1000;
  if (psec != sec)
  {
    psec = sec;
    fps = frame_count;
    frame_count = 0;
  }

  app.update(delta_time);

  frame_count++;
}

#if defined(ESP_PLATFORM)
#include "esp_flash.h"
#include "esp_log.h"

void print_memory_info(void)
{
  // Get total and free sizes for different memory types
  multi_heap_info_t info;

  // 1. Internal SRAM (IRAM/DRAM) - MALLOC_CAP_INTERNAL
  heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
  ESP_LOGI("Main", "\n--- Internal SRAM (IRAM/DRAM) ---\n");
  ESP_LOGI("Main", "Total: %d KB\n", (info.total_free_bytes + info.total_allocated_bytes) / 1024);
  ESP_LOGI("Main", "Free: %d KB\n", info.total_free_bytes / 1024);
  ESP_LOGI("Main", "Largest Free Block: %d KB\n", info.largest_free_block / 1024);

  // 2. External PSRAM (SPIRAM) - MALLOC_CAP_SPIRAM
  if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > 0)
  {
    heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
    ESP_LOGI("Main", "\n--- External PSRAM (SPIRAM) ---\n");
    ESP_LOGI("Main", "Total: %d KB\n", (info.total_free_bytes + info.total_allocated_bytes) / 1024);
    ESP_LOGI("Main", "Free: %d KB\n", info.total_free_bytes / 1024);
    ESP_LOGI("Main", "Largest Free Block: %d KB\n", info.largest_free_block / 1024);
  }
  else
  {
    ESP_LOGI("Main", "\n--- External PSRAM (SPIRAM) ---\n");
    ESP_LOGI("Main", "PSRAM is not enabled or not detected.\n");
  }

  // 3. Flash Size (Storage, not RAM)
  ESP_LOGI("Main", "\n--- Flash (Storage) ---\n");

  uint32_t flash_size;
  auto res = esp_flash_get_size(NULL, &flash_size);

  if (res == ESP_OK)
  {
    ESP_LOGI("Main", "Total Flash Size: %d MB\n", (int)flash_size / (1024 * 1024));
  }
  else
  {
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
#endif

void setup(void)
{
  setup_display();

#if defined(ESP_PLATFORM)
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

  print_memory_info();
#else
  _background.setTextSize(2);
  _background.setColorDepth(8);
  _background.createSprite(lcd.width(), lcd.height());
  _background.clear(lcd.color332(80, 80, 80));
#endif

  app.setup();
}

void loop(void)
{
#if defined(ESP_PLATFORM)
  if (mp3->isRunning())
  {
    if (!mp3->loop())
    {
      mp3->stop();
    }
  }
#endif
  mainfunc();
  drawfunc();
}