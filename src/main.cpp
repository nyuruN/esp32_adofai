
#include "events.h"
#include "rust_typedef.h"
#include "beatmap.h"
#include "lgfx.h"
#include "data.h"
#include "app.h"

App app;

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
  void set_background_jpg(u8 *jpg, u32 length) { BeatmapPlayer::background.drawJpg(jpg, length); }
  EMSCRIPTEN_KEEPALIVE
  void toggle_autohit() { BeatmapPlayer::autohit = !BeatmapPlayer::autohit; }
  EMSCRIPTEN_KEEPALIVE
  void play() { BeatmapPlayer::begin(); }
  EMSCRIPTEN_KEEPALIVE
  void hit() { app.input(Input::Press);; }
  EMSCRIPTEN_KEEPALIVE
  void up() { app.input(Input::Up); }
  EMSCRIPTEN_KEEPALIVE
  void left() { app.input(Input::Left); }
  EMSCRIPTEN_KEEPALIVE
  void down() { app.input(Input::Down); }
  EMSCRIPTEN_KEEPALIVE
  void right() { app.input(Input::Right); }
};
#endif

// Auxiliary variables
u64 pmillis = 0;
u32 sec, psec;
u32 fps = 0, frame_count = 0;
u32 draw_count = 0;


void drawfunc(void)
{
  LGFX_Sprite *sprite;
  std::size_t flip = draw_count & 1;
  sprite = &(_sprites[flip]);

  app.render(sprite);

  if (false) {
    // Debug info
    sprite->setTextColor(TFT_WHITE);
    sprite->setCursor(0, 0);
    sprite->printf("fps:%d", (int)fps);
    sprite->setCursor(0, 20);
    sprite->printf("dir:%d", (int)BeatmapPlayer::current_angle);
    sprite->setCursor(0, 40);
    sprite->printf("next:%d", (int)BeatmapPlayer::angleData[BeatmapPlayer::current_floor]);
    sprite->setCursor(0, 60);
    sprite->printf("prog:%d", (int)BeatmapPlayer::angle_progress);
    sprite->setCursor(0, 80);
    sprite->printf("angle:%d", (int)BeatmapPlayer::angle_next);
    sprite->setCursor(0, 100);
    sprite->printf("floor:%d", (int)BeatmapPlayer::current_floor);
    sprite->setCursor(0, 120);
    sprite->printf("bpm:%.1f", BeatmapPlayer::bpm);
    sprite->setCursor(0, 140);
    sprite->printf("pE:%d", (int)beatmap_events.p_events);
    sprite->setCursor(0, 160);
    sprite->printf("pCE:%d", (int)beatmap_events.active_count);
    sprite->setCursor(0, 180);
    sprite->printf("zoom:%.2f", 1.0f / BeatmapPlayer::DrawData::_zoom);
    sprite->setCursor(0, 200);
    sprite->printf("rot:%.2f", BeatmapPlayer::DrawData::_rotation);
    sprite->setCursor(0, 220);
    sprite->printf("%.2f,%.2f", (BeatmapPlayer::DrawData::_camera_x - BeatmapPlayer::camera_x), (BeatmapPlayer::DrawData::_camera_y - BeatmapPlayer::camera_y));

    sprite->setCursor(185, 0);
    sprite->printf("tiles: % 3d", BeatmapPlayer::tileCount);
    sprite->setCursor(185, 20);
    sprite->printf("events:% 3d", beatmap_events.event_buf_size);
    sprite->setCursor(185, 40);
    sprite->printf("cMode:% 3d", BeatmapPlayer::camera_mode);
    sprite->setCursor(185, 60);
    sprite->printf("trans:% 1.2f", BeatmapPlayer::transition);
    sprite->setCursor(185, 80);
    sprite->printf("%.2f,%.2f", (BeatmapPlayer::anchor_x), BeatmapPlayer::anchor_y);
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
  // TODO: Temporary fix for synchronous upload taking too long
  // Otherwise a too large delta_time might invalidate the app and crash
  if (delta_time > 1) delta_time = 0;

  app.update(delta_time);

  frame_count++;
}

#if defined(ESP_PLATFORM)
#include <LittleFS.h>
#include <Arduino.h>

// We need more than the 8K default with other things running, so just double to avoid issues.
// Opus codes uses *lots* of stack variables in the internal decoder instead of a global working chunk
SET_LOOP_TASK_STACK_SIZE(16 * 1024);  // 16KB
// On the Pico this is already taken care of using the built-in NONTHREADSAFE_PSEUDOSTACK in the config file.

#include "debug.h"
#endif

void setup(void)
{
  setup_display();

  BeatmapPlayer::background.setTextSize(2);
  BeatmapPlayer::background.setColorDepth(8);

#if defined(ESP_PLATFORM)
  LittleFS.begin(true, "/littlefs", 10, "littlefs");
  BeatmapPlayer::background.setPsram(true);
  BeatmapPlayer::background.createSprite(lcd.width(), lcd.height());
  BeatmapPlayer::background.drawPngFile("/littlefs/bg.png", 0, 0, lcd.width(), lcd.height());

  print_memory_info();
#else
  BeatmapPlayer::background.createSprite(lcd.width(), lcd.height());
  BeatmapPlayer::background.clear(lcd.color332(80, 80, 80));
#endif

  app.setup();
}

void loop(void)
{
  mainfunc();
  drawfunc();
}