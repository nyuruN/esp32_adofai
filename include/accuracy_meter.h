#pragma once

#include "lgfx.h"
#include "rust_typedef.h"

class AccuracyMeter
{
public:
  static inline const u32 BAR_WIDTH = 180;
  static inline const u32 PERFECT_WIDTH = BAR_WIDTH * (30.0 / 90.0);
  static inline const u32 ELPERFECT_WIDTH = BAR_WIDTH * (45.0 / 90.0);
  static inline const u32 EL_WIDTH = BAR_WIDTH * (60.0 / 90.0);
  // static inline const u32 MISS_WIDTH = BAR_WIDTH * (90.0 / 90.0);
  static inline const u32 MISS_WIDTH = EL_WIDTH + 8;
  static inline const u32 Y_START = 220;
  static inline const u32 HEIGHT = 7;
  static inline const u8 PERFECT_COLOR = lcd.color332(40, 240, 40);
  static inline const u8 ELPERFECT_COLOR = lcd.color332(255, 255, 50);
  static inline const u8 EL_COLOR = lcd.color332(255, 150, 0);
  static inline const u8 MISS_COLOR = lcd.color332(255, 0, 0);

  struct Data
  {
    float accuracy;
    u8 timer; // Also transparency
  };

  static inline const u32 SAMPLE_COUNT = 8;

  u32 p_data = 0;
  Data data[SAMPLE_COUNT] = {};

  void render(LGFX_Sprite *sprite)
  {
    const i32 center = sprite->width() / 2;

    sprite->fillRect(center - PERFECT_WIDTH / 2, Y_START, PERFECT_WIDTH, HEIGHT, PERFECT_COLOR);

    sprite->fillRect(center - ELPERFECT_WIDTH / 2, Y_START, (ELPERFECT_WIDTH - PERFECT_WIDTH) / 2, HEIGHT, ELPERFECT_COLOR);
    sprite->fillRect(center + PERFECT_WIDTH / 2, Y_START, (ELPERFECT_WIDTH - PERFECT_WIDTH) / 2, HEIGHT, ELPERFECT_COLOR);

    sprite->fillRect(center - EL_WIDTH / 2, Y_START, (EL_WIDTH - ELPERFECT_WIDTH) / 2, HEIGHT, EL_COLOR);
    sprite->fillRect(center + ELPERFECT_WIDTH / 2, Y_START, (EL_WIDTH - ELPERFECT_WIDTH) / 2, HEIGHT, EL_COLOR);

    sprite->fillRect(center - MISS_WIDTH / 2, Y_START, (MISS_WIDTH - EL_WIDTH) / 2, HEIGHT, MISS_COLOR);
    sprite->fillRect(center + EL_WIDTH / 2, Y_START, (MISS_WIDTH - EL_WIDTH) / 2, HEIGHT, MISS_COLOR);

    sprite->fillRect(center + EL_WIDTH / 2, Y_START, (MISS_WIDTH - EL_WIDTH) / 2, HEIGHT, MISS_COLOR);

    // Center line
    // sprite->fillRect(center - 1, Y_START - 2, 2, HEIGHT + 4, TFT_WHITE);

    // Draw samples
    float avg = 0;
    for (u8 i = 0; i < SAMPLE_COUNT; i++)
    {
      avg += data[wrap(p_data + i)].accuracy;
      if (data[wrap(p_data + i)].timer == 0)
        continue;
      sprite->fillRectAlpha(center - data[wrap(p_data + i)].accuracy - 1, Y_START - 2, 2, HEIGHT + 4, data[wrap(p_data + i)].timer, TFT_WHITE);
    }
    avg /= SAMPLE_COUNT;

    sprite->setCursor(center - 4 - 1 - avg, Y_START + HEIGHT + 4);
    sprite->setTextColor(TFT_WHITE);
    sprite->print('^');
  }
  static inline i8 wrap(i8 idx)
  {
    if (idx < 0)
      idx += SAMPLE_COUNT;
    idx = idx % SAMPLE_COUNT;
    return idx;
  }
  void update(float delta_time)
  {
    // u32 delta_millis = delta_time * 1000 / 2.0;
    u32 delta_millis = delta_time * 500;

    // u8 inc = 0;
    for (u8 i = 0; i < SAMPLE_COUNT; i++)
    {
      if (data[wrap(p_data + i)].timer >= delta_millis)
        data[wrap(p_data + i)].timer -= delta_millis;
      else
      {
        data[wrap(p_data + i)].timer = 0;
        // inc++;
      }
    }
    // Remove samples
    // p_data = wrap(p_data + inc);
  }
  void hit(float angle_diff)
  {
    if (angle_diff > 90.0)
      angle_diff = 90.0;
    if (angle_diff < -90.0)
      angle_diff = -90.0;
    u8 idx = wrap(p_data + SAMPLE_COUNT);
    data[idx] = Data{
        .accuracy = angle_diff,
        .timer = 255,
    };
    p_data = wrap(p_data + 1);
  }
};
