
#include "beatmap.h"
#include "events.h"
#include "camera_events.h"
#include "data.h"

namespace Beatmap {

template <typename T>
void camera_transform(T* x, T* y) {
  *x = ((*x - DrawData::_camera_x) * DrawData::rcos - (*y - DrawData::_camera_y) * DrawData::rsin) * DrawData::_zoom + DrawData::width / 2;
  *y = ((*x - DrawData::_camera_x) * DrawData::rsin + (*y - DrawData::_camera_y) * DrawData::rcos) * DrawData::_zoom * (-1) + DrawData::width / 2;
}

void prepare() {

}
void render(LGFX_Sprite* sprite) {
  sprite->clear(lcd.color332(80, 80, 80));

  DrawData::sprite = sprite;
  DrawData::width = sprite->width();
  DrawData::height = sprite->height();
  DrawData::_zoom = zoom * pulse;
  DrawData::_camera_x = camera_x + offset_x;
  DrawData::_camera_y = camera_y + offset_y;
  DrawData::_rotation = rotation;

  CameraEvents::apply(&DrawData::_camera_x, &DrawData::_camera_y, &DrawData::_rotation, &DrawData::_zoom);

  DrawData::rcos = cos(DrawData::_rotation * 3.14159 / 180);
  DrawData::rsin = sin(DrawData::_rotation * 3.14159 / 180);

  draw_tiles();
  draw_planets();
}
void hit() {
  // Allow angle_progress to go in negatives to compensate off timing
  // + diff: early hit
  // - diff: late hit
  float diff = angle_next - angle_progress;

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

  current_floor++;
  current_planet = !current_planet;
  angle_progress = -diff;
  current_angle = angle_data[current_floor - 1] - 180.0 + diff * (current_direction ? -1 : 1); // 180 for planet switch, diff for compensation
  if (tile_data[current_floor] & TILE_TWIRL) current_direction = !current_direction;

  // The correct angle distance
  angle_next = angle_dst(angle_data[current_floor - 1] - 180, angle_data[current_floor]);
  next_position();

  // Camera pulse
  pulse = 1.02;

  // traverse events
  Events::next_floor();
}
void update(float delta_time) {
  { // auto hit logic
    if (angle_progress > angle_next && !(current_floor >= (TILE_BUF_SIZE - 1))) {
      hit();
    }
  }

  // Angle updates
  const float angle_delta = 180.0 * (bpm / 60.0) * delta_time * (current_direction ? 1 : -1);
  current_angle += angle_delta;
  angle_progress += abs(angle_delta);
  // Angle wrap around
  if (current_angle < 0.0) current_angle += 360.0;
  if (current_angle >= 360.0) current_angle -= 360.0;

  // Event dispatch
  Events::update();

  // Camera smoothing
  camera_x += (positions[(p_positions + 1) % P_BUF_SIZE][0] - camera_x) * delta_time * 1.0;
  camera_y += (positions[(p_positions + 1) % P_BUF_SIZE][1] - camera_y) * delta_time * 1.0;
  // Camera pulse
  pulse = pulse + (1 - pulse) * delta_time * 4.0;
  CameraEvents::update(delta_time * 1000);
}
void draw_planets() {
  i16 attractor_x = positions[p_positions][0];
  i16 attractor_y = positions[p_positions][1];
  i16 orbit_x = attractor_x + cos(current_angle * 3.14159 / 180) * beat_radius;
  i16 orbit_y = attractor_y + sin(current_angle * 3.14159 / 180) * beat_radius;

  camera_transform(&attractor_x, &attractor_y);
  camera_transform(&orbit_x, &orbit_y);


  // Draw planets
  DrawData::sprite->fillCircle(attractor_x, attractor_y, planet_size * DrawData::_zoom, (current_planet ? p1_color : p0_color));
  DrawData::sprite->fillCircle(orbit_x, orbit_y, planet_size * DrawData::_zoom, (!current_planet ? p1_color : p0_color));
}
void draw_tiles() {
  DrawData::sprite->setTextColor(TFT_BLACK);

  // Draw tiles
  for (i8 i = 0; i < P_BUF_SIZE; i++) {
    const u16 floor_idx = current_floor + (P_BUF_SIZE - 1 - P_OFFSET) - i;
    if (floor_idx < 0 || floor_idx >= TILE_BUF_SIZE) continue; // Clip invalid floors

    i8 idx = (i8)p_positions + (P_BUF_SIZE - 1 - P_OFFSET) - i;
    if (idx < 0) idx += P_BUF_SIZE;
    else idx = idx % P_BUF_SIZE;

    u8 _tile_color = tile_color;
    u8 border_color = lcd.color332(20, 20, 20);
    const u8 _tile_data = tile_data[floor_idx];

    if (_tile_data & TILE_TWIRL) {
      border_color = lcd.color332(250, 50, 50);
    }   
    if (_tile_data & TILE_SPEEDDOWN) {
      _tile_color = lcd.color332(80, 80, 200);
    }   
    if (_tile_data & TILE_SPEEDUP) {
      _tile_color = lcd.color332(200, 80, 80);
    }   
    if (_tile_data & TILE_CHECKPOINT) {
      border_color = lcd.color332(50, 250, 50);
    }

    //i16 tile_x = positions[idx][0];
    //i16 tile_y = positions[idx][1];
    float tile_x = positions[idx][0];
    float tile_y = positions[idx][1];

    float c = cos(angle_data[floor_idx] * M_PI / 180.0f);
    float s = sin(angle_data[floor_idx] * M_PI / 180.0f);
    float mid_x = tile_x + c * beat_radius / 2.0f; 
    float mid_y = tile_y + s * beat_radius / 2.0f; 
    // b r corner
    float p0x = tile_x + s * tile_size;
    float p0y = tile_y + -c * tile_size;
    // b l corner
    float p1x = tile_x - s * tile_size;
    float p1y = tile_y - -c * tile_size;
    // t r corner
    float p2x = mid_x + s * tile_size;
    float p2y = mid_y + -c * tile_size;
    // t l corner
    float p3x = mid_x - s * tile_size;
    float p3y = mid_y - -c * tile_size;

    float pc = cos((angle_data[floor_idx - 1] - 180) * M_PI / 180.0f);
    float ps = sin((angle_data[floor_idx - 1] - 180) * M_PI / 180.0f);
    float pmid_x = tile_x + pc * beat_radius / 2.0f; 
    float pmid_y = tile_y + ps * beat_radius / 2.0f; 
    // b r corner
    float p4x = tile_x + ps * tile_size;
    float p4y = tile_y + -pc * tile_size;
    // b l corner
    float p5x = tile_x - ps * tile_size;
    float p5y = tile_y - -pc * tile_size;
    // t r corner
    float p6x = pmid_x + ps * tile_size;
    float p6y = pmid_y + -pc * tile_size;
    // t l corner
    float p7x = pmid_x - ps * tile_size;
    float p7y = pmid_y - -pc * tile_size;

    camera_transform(&tile_x, &tile_y);
    camera_transform(&p0x, &p0y);
    camera_transform(&p1x, &p1y);
    camera_transform(&p2x, &p2y);
    camera_transform(&p3x, &p3y);
    camera_transform(&mid_x, &mid_y);
    camera_transform(&p4x, &p4y);
    camera_transform(&p5x, &p5y);
    camera_transform(&p6x, &p6y);
    camera_transform(&p7x, &p7y);
    camera_transform(&pmid_x, &pmid_y);

    //DrawData::sprite->fillCircle(tile_x, tile_y, tile_size * DrawData::_zoom + 1, border_color);
    //DrawData::sprite->fillCircle(tile_x, tile_y, (tile_size * 1.15) * DrawData::_zoom, border_color);
    DrawData::sprite->fillCircle(tile_x, tile_y, tile_size * DrawData::_zoom, _tile_color);
    
    DrawData::sprite->fillTriangle(p0x, p0y, p1x, p1y, p2x, p2y, _tile_color);
    DrawData::sprite->fillTriangle(p1x, p1y, p2x, p2y, p3x, p3y, _tile_color);
    DrawData::sprite->fillTriangle(p4x, p4y, p5x, p5y, p6x, p6y, _tile_color);
    DrawData::sprite->fillTriangle(p5x, p5y, p6x, p6y, p7x, p7y, _tile_color);
    /*
    DrawData::sprite->drawLine(p0x, p0y, p2x, p2y, border_color);
    DrawData::sprite->drawLine(p1x, p1y, p3x, p3y, border_color);
    DrawData::sprite->drawLine(p4x, p4y, p6x, p6y, border_color);
    DrawData::sprite->drawLine(p5x, p5y, p7x, p7y, border_color);
    */

    DrawData::sprite->drawLine(p2x, p2y, p3x, p3y, border_color);
    
  }
}
// Init positions, traverse tilemap relative to current floor
void init_positions(void) {
  positions[p_positions][0] = 0;
  positions[p_positions][1] = 0;
  for (u8 i = 1; i <= (P_BUF_SIZE - 1 - P_OFFSET); i++) {
    if (current_floor + i >= TILE_BUF_SIZE) break;
    u8 idx = (p_positions + i) % P_BUF_SIZE;
    positions[idx][0] = positions[(idx - (i8)1) + (1 > idx ? P_BUF_SIZE : 0)][0] + cos(angle_data[current_floor + i - 1] * 3.14159 / 180) * beat_radius;
    positions[idx][1] = positions[(idx - (i8)1) + (1 > idx ? P_BUF_SIZE : 0)][1] + sin(angle_data[current_floor + i - 1] * 3.14159 / 180) * beat_radius;
  }
  for (u8 i = 1; i <= P_OFFSET; i++) {
    if ((int)current_floor - i < 0) break;
    u8 idx = ((i8)p_positions - i) + (i > p_positions ? P_BUF_SIZE : 0);
    positions[idx][0] = positions[(idx + 1) % P_BUF_SIZE][0] + cos((angle_data[current_floor - i] + 180) * 3.14159 / 180) * beat_radius;
    positions[idx][1] = positions[(idx + 1) % P_BUF_SIZE][1] + sin((angle_data[current_floor - i] + 180) * 3.14159 / 180) * beat_radius;
  }
}
void next_position(void) {
  p_positions = (p_positions + 1) % P_BUF_SIZE;
  u8 idx = (p_positions + (P_BUF_SIZE - 1 - P_OFFSET)) % P_BUF_SIZE;
  u8 prev_idx = ((i8)idx - 1) + (1 > idx ? P_BUF_SIZE : 0);
  positions[idx][0] = positions[prev_idx][0] + cos(angle_data[current_floor + (P_BUF_SIZE - 2 - P_OFFSET)] * 3.14159 / 180) * beat_radius;
  positions[idx][1] = positions[prev_idx][1] + sin(angle_data[current_floor + (P_BUF_SIZE - 2 - P_OFFSET)] * 3.14159 / 180) * beat_radius;
}
float angle_dst(float angle_from, float angle_to) {
  if (angle_from < 0.0) angle_from += 360.0;
  if (angle_to < 0.0) angle_to += 360.0;
  if (current_direction) { // CCW
    if (angle_from < angle_to) {
      return angle_to - angle_from;
    } else {
      return angle_to - angle_from + 360;
    } 
  } else {
    if (angle_from < angle_to) {
      return angle_from - angle_to + 360;
    } else {
      return angle_from - angle_to;
    }
  }
}
void clear(bool erase_data) {
  // Reset beatmap state
  current_floor = 0;
  current_events = 0;
  current_angle = 180;
  current_planet = 0; // 0 = p0
  current_direction = 0; // 0 = cw
  angle_progress = 0;
  angle_next = 0;
  is_playing = false;

  // Camera
  camera_x = 0.0;
  camera_y = 0.0;
  offset_x = 0.0;
  offset_y = 0.0;
  zoom = 1.4;
  rotation = 0;
  pulse = 1.0;

  // Pointers
  Events::p_events = 0;
  Beatmap::p_positions = 0;
  CameraEvents::count = 0;
  
  if (!erase_data) return;

  // Buffer sizes
  Events::event_buf_size = 0;
  Beatmap::TILE_BUF_SIZE = 0;

  // Buffers
  Beatmap::angle_data = nullptr;
  Beatmap::tile_data = nullptr;
  Events::events = nullptr;

}
void set_event_data(u8* event_buf, u32 length) {
  Events::events = reinterpret_cast<Event*>(event_buf);
  Events::event_buf_size = length;
}
void begin() {
  angle_next = angle_dst(current_angle, angle_data[current_floor]);
  is_playing = true;
  init_positions();
}

}
