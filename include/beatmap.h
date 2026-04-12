#pragma once

#include "rust_typedef.h"
#include "lgfx.h"

namespace Beatmap {

// Variables

// Flags
constexpr u8 TILE_TWIRL =       0b00000001;
constexpr u8 TILE_SPEEDUP =     0b00000010;
constexpr u8 TILE_SPEEDDOWN =   0b00000100;
constexpr u8 TILE_CHECKPOINT =  0b00001000;

inline u16 TILE_BUF_SIZE = 0;

// Constants
constexpr u16 P_OFFSET = 24;
constexpr u16 P_BUF_SIZE = 64;
constexpr u8 beat_radius = 25;
constexpr i32 planet_size = 8;
constexpr i32 tile_size = 10;
constexpr u8 p0_color = lcd.color332(50, 40, 255);
constexpr u8 p1_color = lcd.color332(255, 50, 40);
constexpr u8 tile_color = lcd.color332(200, 200, 200);

// States
inline u32 current_floor = 0;
inline u8 current_events = 0;
inline float current_angle = 0;
inline bool current_planet = 0; // 0 = p0
inline bool current_direction = 0; // 0 = cw
inline u16 bpm = 227;
inline float angle_progress = 0;
inline float angle_next = 0;
inline bool is_playing = false;

// Tilemap
inline u8 p_positions = 0; // Represents current_floor 
inline i16 positions[P_BUF_SIZE][2] = {};

// Camera
inline float camera_x;
inline float camera_y;
inline float offset_x = 0.0;
inline float offset_y = 0.0;
inline float zoom = 1.0;
inline float rotation = 0;
inline float pulse = 1.0;

// Data recalculated on every draw call
namespace DrawData {
	inline u16 width = lcd.width();
	inline u16 height = lcd.height();
	inline LGFX_Sprite* sprite;
	inline float _camera_x;
	inline float _camera_y;
	inline float _zoom;
	inline float _rotation;
	inline float rcos;
	inline float rsin;
};

// Functions

extern void prepare();
extern void render(LGFX_Sprite* sprite);
extern void update(float delta_time);
extern void hit();
extern void draw_planets();
extern void draw_tiles();
extern void init_positions();
extern void next_position();
// Calculate angle distance based on direction
extern float angle_dst(float angle_from, float angle_to);
// Reset beatmap state
extern void clear();
extern void begin();

};

namespace Beatmap {

// Beatmap data

inline i16* angle_data = nullptr;
inline u8* tile_data = nullptr;

inline void set_beatmap_data(u8* angle_buf, u8* tile_buf, u32 length) {
	angle_data = reinterpret_cast<i16*>(angle_buf);
	tile_data = tile_buf;
	TILE_BUF_SIZE = length;
}
inline void set_bpm(float v) { bpm = v; }
extern void set_event_data(u8* event_buf, u32 length);

};