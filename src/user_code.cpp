
// If you write this, you can use drawBmpFile / drawJpgFile / drawPngFile
// #include <stdio.h>

// If you write this, you can use drawBmpUrl / drawJpgUrl / drawPngUrl ( for Windows )
// #include <windows.h>
// #include <winhttp.h>
// #pragma comment (lib, "winhttp.lib")

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>

#include "events.h"
#include "rust_typedef.h"
#include "beatmap.h"
#include "camera_events.h"
#include "lgfx.h"
#include "data.h"

// Auxiliary variables
static std::uint64_t pmillis = 0;
static std::uint32_t _fps = 0;
static std::uint32_t sec, psec;
static std::uint32_t fps = 0, frame_count = 0;
bool _is_running;
std::uint32_t _draw_count;
std::uint32_t _loop_count;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
extern "C" {
  EMSCRIPTEN_KEEPALIVE
  void clear(bool erase_data) { Beatmap::clear(erase_data); }
  EMSCRIPTEN_KEEPALIVE
  void set_beatmap_data(u8* angle_buf, u8* tile_buf, u32 length) { Beatmap::set_beatmap_data(angle_buf, tile_buf, length); }
  EMSCRIPTEN_KEEPALIVE
  void set_bpm(float v) { Beatmap::set_bpm(v); }
  EMSCRIPTEN_KEEPALIVE
  void set_event_data(u8* event_buf, u32 length) { Beatmap::set_event_data(event_buf, length); }
  EMSCRIPTEN_KEEPALIVE
  void play() { Beatmap::begin(); }
  EMSCRIPTEN_KEEPALIVE
  void hit() { Beatmap::hit(); }
};
#endif

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

  frame_count++;
  _loop_count++;
  _fps = fps;
}

void setup(void) {
  setup_display();

  Beatmap::clear();

  // Assign beatmap data
  Beatmap::set_beatmap_data(Data::angleData, Data::tileData, Data::tileCount);
  Beatmap::set_bpm(227);
  Beatmap::set_bpm(20);
  Beatmap::set_event_data(Data::eventData, Data::eventCount);

  Beatmap::begin();

  _is_running = true;
  _draw_count = 0;
  _loop_count = 0;
}

void loop(void) {
  mainfunc();
  drawfunc();
}