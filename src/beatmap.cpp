
#include "beatmap.h"
#include "events.h"
#include "data.h"
#include "accuracy_meter.h"
#include "tilemap.h"

namespace BeatmapPlayer
{

  void prepare()
  {
  }
  void render(LGFX_Sprite *sprite)
  {
    background.pushSprite(sprite, 0, 0);

    DrawData::sprite = sprite;
    DrawData::width = sprite->width();
    DrawData::height = sprite->height();
    DrawData::_zoom = zoom * pulse;
    DrawData::_camera_x = camera_x + offset_x;
    DrawData::_camera_y = camera_y + offset_y;
    DrawData::_rotation = rotation;

    beatmap_events.apply(&DrawData::_camera_x, &DrawData::_camera_y, &DrawData::_rotation, &DrawData::_zoom);

    DrawData::rcos = cos(DrawData::_rotation * 3.14159 / 180);
    DrawData::rsin = sin(DrawData::_rotation * 3.14159 / 180);

    tilemap.render(sprite);
    draw_planets();
    meter.render(sprite);
  }
  void hit()
  {
    // Allow angle_progress to go in negatives to compensate off timing
    // + diff: early hit
    // - diff: late hit
    float diff = angle_next - angle_progress;

    meter.hit(diff);
    /*
    if (abs(diff) > 60.0) {
      printf("Miss/Loss: %.2f\n", diff);
      return;
    } else if (abs(diff) > 45.0) {
      printf("Early/Late: %.2f\n", diff);
    } else if (abs(diff) > 30.0) {
      printf("E/LPerfect: %.2f\n", diff);
    } else {
      printf("perfect: %.2f\n", diff);
    }
    */

    current_floor++;
    current_planet = !current_planet;
    angle_progress = -diff;
    current_angle = angleData[current_floor - 1] - 180.0 + diff * (current_direction ? -1 : 1); // 180 for planet switch, diff for compensation
    if (tileData[current_floor] & TILE_TWIRL)
      current_direction = !current_direction;

    // The correct angle distance
    angle_next = angle_dst(angleData[current_floor - 1] - 180, angleData[current_floor]);
    tilemap.next_tile();

    // Camera pulse
    pulse = 1.02;

    // traverse events
    beatmap_events.next_floor();
  }
  void update(float delta_time)
  {
    { // auto hit logic
      if (angle_progress > angle_next && !(current_floor >= (tileCount - 1)))
      {
        hit();
      }
    }

    // Angle updates
    const float angle_delta = 180.0 * (bpm / 60.0) * delta_time * (current_direction ? 1 : -1);
    current_angle += angle_delta;
    angle_progress += abs(angle_delta);
    // Angle wrap around
    if (current_angle < 0.0)
      current_angle += 360.0;
    if (current_angle >= 360.0)
      current_angle -= 360.0;

    // Event dispatch
    beatmap_events.update(delta_time * 1000);
    meter.update(delta_time);

    // Camera smoothing
    
    if (camera_mode == RelativeTo::Player) {
      camera_x += (tilemap.get_relative(0).x - camera_x) * delta_time * 0.01 * bpm;
      camera_y += (tilemap.get_relative(0).y - camera_y) * delta_time * 0.01 * bpm;
    } else if (camera_mode == RelativeTo::Tile) {
      camera_x = prev_anchor_x + (anchor_x - prev_anchor_x) * transition;
      camera_y = prev_anchor_y + (anchor_y - prev_anchor_y) * transition;
    }

    // Camera pulse
    pulse = pulse + (1 - pulse) * delta_time * 4.0;
  }
  void draw_planets()
  {
    i16 attractor_x = tilemap.get_relative(0).x;
    i16 attractor_y = tilemap.get_relative(0).y;
    i16 orbit_x = attractor_x + cos(current_angle * 3.14159 / 180) * beat_radius;
    i16 orbit_y = attractor_y + sin(current_angle * 3.14159 / 180) * beat_radius;

    camera_transform(&attractor_x, &attractor_y);
    camera_transform(&orbit_x, &orbit_y);

    // Draw planets
    DrawData::sprite->fillCircle(attractor_x, attractor_y, planet_size * DrawData::_zoom, (current_planet ? p1_color : p0_color));
    DrawData::sprite->fillCircle(orbit_x, orbit_y, planet_size * DrawData::_zoom, (!current_planet ? p1_color : p0_color));
  }
  float angle_dst(float angle_from, float angle_to)
  {
    if (angle_from < 0.0)
      angle_from += 360.0;
    if (angle_to < 0.0)
      angle_to += 360.0;
    if (angle_from == angle_to)
      return 360.0;
    if (current_direction)
    { // CCW
      if (angle_from < angle_to)
      {
        return angle_to - angle_from;
      }
      else
      {
        return angle_to - angle_from + 360;
      }
    }
    else
    {
      if (angle_from < angle_to)
      {
        return angle_from - angle_to + 360;
      }
      else
      {
        return angle_from - angle_to;
      }
    }
  }
  void clear(bool erase_data)
  {
    // Reset beatmap state
    current_floor = 0;
    current_events = 0;
    current_angle = 180;
    current_planet = 0;    // 0 = p0
    current_direction = 0; // 0 = cw
    angle_progress = 0;
    angle_next = 0;

    // Camera
    camera_x = 0.0;
    camera_y = 0.0;
    offset_x = 0.0;
    offset_y = 0.0;
    zoom = 1.4;
    rotation = 0;
    pulse = 1.0;

    beatmap_events.clear();
    tilemap.clear();

    // Pointers

    if (!erase_data)
      return;

    // Buffer sizes
    BeatmapPlayer::tileCount = 0;

    // Buffers
    BeatmapPlayer::angleData = nullptr;
    BeatmapPlayer::tileData = nullptr;
  }
  void set_event_data(u8 *event_buf, u32 length)
  {
    beatmap_events.event_buf_size = length;
    beatmap_events.events = reinterpret_cast<Event *>(event_buf);
  }
  void begin()
  {
    angle_next = angle_dst(current_angle, angleData[current_floor]);
    tilemap.init_tiledrawdata();
  }

}
