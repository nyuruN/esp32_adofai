#include <Arduino.h>
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
static LGFX_Sprite _background;

// Auxiliary variables
static std::uint64_t pmillis = 0;
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

enum class VisualInfo {
  None = 0,
  Twirl = 1,
  SpeedUp = 2,
  SpeedDown = 3,
};
enum class EventType : u8 {
  MoveCamera = 0,
  ShakeScreen = 1,
};

enum class EaseType : u8 {
  Linear = 0,
  EaseIn = 1,
  EaseOut = 2,
  EaseInOut = 3,
};

struct ShakeScreen {
  u32 floor;
  u32 duration;
  u16 intensity;
  u8 strength;
  EaseType ease;
};
struct MoveCamera {
  u32 floor;
  u32 duration;
  u16 zoom;
  i16 rotation;
  i8 pos_x;
  i8 pos_y;
  EaseType ease;
};
struct SetSpeed {
  u32 floor;
  float bpm;
  bool is_multiplier;
};

struct Event {
  EventType type;
  union {
    MoveCamera move_camera;
    ShakeScreen shake_screen;
    SetSpeed set_speed;
  };
};

static i16 angle_data[32] = { 0, 0, 90, 90, 180, 180, 180, 90, 0, 90, -135, 0, 90, 180, 90, 0, 0, 0, 0, 0, 270, 225, 90, 0, 0, 0, 0, 45, 90, 180, 180, 180, };
static bool twirl_data[32] = { false };
static Event events[64];
static u32 current_floor = 0;
static float current_angle = 0;
static bool current_planet = 0; // 0 = p0
static bool current_direction = 0; // 0 = cw
static u8 p0_color = lcd.color332(50, 40, 255);
static u8 p1_color = lcd.color332(255, 50, 40);
static u8 tile_color = lcd.color332(240, 240, 240);
static u16 bpm = 120;
const u8 beat_radius = 30;

static u8 p_positions = 0; // Represents current_floor 
static u16 positions[16][2] = {};

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


// Init positions, traverse tilemap relative to current floor
void init_positions(void) {
  positions[p_positions][0] = lcd.width() / 2;
  positions[p_positions][1] = lcd.height() / 2;
  for (u8 i = 1; i < 8; i++) {
    if (current_floor + i >= 32) break;
    u8 idx = p_positions + i;
    if (idx >= 16) idx -= 16; // Wrap around
    positions[idx][0] = positions[idx - 1][0] + cos(angle_data[current_floor + i] * 3.14159 / 180) * beat_radius;
    positions[idx][1] = positions[idx - 1][1] - sin(angle_data[current_floor + i] * 3.14159 / 180) * beat_radius;
  }
  for (u8 i = 1; i < 8; i++) {
    if (current_floor - i < 0) break;
    u8 idx = p_positions - i;
    // 255 + 16 = 15, wraps around even if idx is 'negative'
    if (idx >= 16) idx += 16;
    positions[idx][0] = positions[idx + 1][0] + cos(angle_data[current_floor - i] * 3.14159 / 180) * beat_radius;
    positions[idx][1] = positions[idx + 1][1] - sin(angle_data[current_floor - i] * 3.14159 / 180) * beat_radius;
  }
}

static void drawfunc(void)
{
  LGFX_Sprite *sprite;

  auto width  = _sprites[0].width();
  auto height = _sprites[0].height();

  std::size_t flip = _draw_count & 1;

  sprite = &(_sprites[flip]);
  sprite->clear();

  _background.pushSprite(sprite, 0, 0);

  u16 center_x = width / 2, center_y = height / 2;
  u16 orbit_x = center_x + cos(current_angle * 3.14159 / 180) * beat_radius, orbit_y = center_y - sin(current_angle * 3.14159 / 180) * beat_radius;

  // Draw tiles
  u16 floor_x = center_x;
  u16 floor_y = center_y;
  sprite->fillCircle(floor_x, floor_y, 15, twirl_data[current_floor] ? lcd.color332(255, 100, 100) : tile_color);
  sprite->fillCircle(floor_x, floor_y, 13, twirl_data[current_floor] ? lcd.color332(255, 100, 100) : tile_color);
  //for (i8 i = -8; i < 8; i++) {
  for (u8 i = 0; i < 16; i++) {
    if (current_floor + i < 0 || current_floor + i >= 32) continue; // Clip invalid floors
    floor_x += cos(angle_data[current_floor + i] * 3.14159 / 180) * beat_radius;
    floor_y -= sin(angle_data[current_floor + i] * 3.14159 / 180) * beat_radius;
    //u8 idx = p_positions + i;
    //if (idx >= 16) idx -= 16; // Wrap around
    //if (positions[idx][0] < 0 - beat_radius || positions[idx][0] >= width + beat_radius) continue; // Clip out of view tiles
    //if (positions[idx][1] < 0 - beat_radius || positions[idx][1] >= height + beat_radius) continue;
    //sprite->fillCircle(positions[idx][0], positions[idx][1], 15, lcd.color332(20, 20, 20));
    //sprite->fillCircle(positions[idx][0], positions[idx][1], 13, twirl_data[current_floor + i] ? lcd.color332(255, 100, 100) : tile_color);
    sprite->fillCircle(floor_x, floor_y, 15, twirl_data[current_floor + i] ? lcd.color332(255, 100, 100) : tile_color);
    sprite->fillCircle(floor_x, floor_y, 13, twirl_data[current_floor + i] ? lcd.color332(255, 100, 100) : tile_color);
  }
  
  // Draw planets
  sprite->fillCircle(center_x, center_y, 10, (current_planet ? p1_color : p0_color));
  sprite->fillCircle(orbit_x, orbit_y, 10, (!current_planet ? p1_color : p0_color));

  // Debug info
  sprite->setCursor(1,1);
  sprite->setTextColor(TFT_BLACK);
  sprite->printf("fps:%d", (int)_fps);
  sprite->setCursor(0,0);
  sprite->setTextColor(TFT_WHITE);
  sprite->printf("fps:%d", (int)_fps);
  sprite->setCursor(1,21);
  sprite->setTextColor(TFT_BLACK);
  sprite->printf("dir:%d", (int)current_angle);
  sprite->setCursor(0,20);
  sprite->setTextColor(TFT_WHITE);
  sprite->printf("dir:%d", (int)current_angle);

  diffDraw(&_sprites[flip], &_sprites[!flip]);
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
    vTaskDelay(1);
  }

  { // hit logic
    float next_angle = angle_data[current_floor];
    if (next_angle < 0) next_angle += 360;
    if (current_angle < 0) current_angle += 360;
    float angle_diff = abs(next_angle - current_angle);

    if (angle_diff < 1.0f) {
      // Simulate hit
      current_floor++;
      //p_positions++;
      current_planet = !current_planet;
      current_angle = angle_data[current_floor] + 180;
      //init_positions();
    }
  }

  float bps = bpm / 60;
  current_angle = fmodf(current_angle + 180 * bps * delta_time * (current_direction ? 1 : -1), 360.0f);

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

  _width = lcd_width << SHIFTSIZE;
  _height = lcd_height << SHIFTSIZE;

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

  init_positions();
  mp3->begin(file, out);

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