#pragma once

#include "rust_typedef.h"
#include "lgfx.h"
#include "accuracy_meter.h"

namespace BeatmapPlayer
{

	// Flags
	constexpr u8 TILE_TWIRL = 0b00000001;
	constexpr u8 TILE_SPEEDUP = 0b00000010;
	constexpr u8 TILE_SPEEDDOWN = 0b00000100;
	constexpr u8 TILE_CHECKPOINT = 0b00001000;

	inline u32 tileCount = 0;

	// Constants
	constexpr u32 beat_radius = 40;
	constexpr u32 planet_size = 9;
	constexpr u32 tile_size = 10;
	constexpr u8 p0_color = lcd.color332(50, 40, 255);
	constexpr u8 p1_color = lcd.color332(255, 50, 40);
	constexpr u8 tile_color = lcd.color332(200, 200, 200);
	constexpr u8 active_tile_color = lcd.color332(230, 230, 230);

	// Player States
	inline u32 current_floor = 0;
	inline u32 current_events = 0;
	inline float current_angle = 0;
	inline bool current_planet = 0;	   // 0 = p0
	inline bool current_direction = 0; // 0 = cw
	inline float bpm = 227;
	inline float angle_progress = 0;
	inline float angle_next = 0;

	// Tilemap
	/*
	struct TileDrawData {
		i16 x;
		i16 y;
	};
	constexpr u32 P_PLAYER_OFFSET = 24;
	constexpr u32 P_TILES = 64;
	inline u32 p_tiledrawdata = 0; // Represents current_floor
	inline TileDrawData tiledrawdata[P_TILES] = {};
	*/

	// Camera
	inline float camera_x;
	inline float camera_y;
	inline float offset_x = 0.0;
	inline float offset_y = 0.0;
	inline float zoom = 1.0;
	inline float rotation = 0;
	inline float pulse = 1.0;

	// Accuracy Meter
	inline AccuracyMeter meter;

	// Data recalculated on every draw call
	namespace DrawData
	{
		inline u32 width = lcd.width();
		inline u32 height = lcd.height();
		inline LGFX_Sprite *sprite;
		inline float _camera_x;
		inline float _camera_y;
		inline float _zoom;
		inline float _rotation;
		inline float rcos;
		inline float rsin;
	};

	// Functions

	extern void prepare();
	extern void render(LGFX_Sprite *sprite);
	extern void update(float delta_time);
	extern void hit();
	extern void draw_planets();
	// Calculate angle distance based on direction
	extern float angle_dst(float angle_from, float angle_to);
	// Reset beatmap state
	extern void clear(bool erase_data = false);
	extern void begin();

	template <typename T>
	void camera_transform(T *x, T *y)
	{
		*x = ((*x - DrawData::_camera_x) * DrawData::rcos - (*y - DrawData::_camera_y) * DrawData::rsin) * DrawData::_zoom + DrawData::width / 2;
		*y = ((*x - DrawData::_camera_x) * DrawData::rsin + (*y - DrawData::_camera_y) * DrawData::rcos) * DrawData::_zoom * (-1) + DrawData::width / 2;
	}

};

namespace BeatmapPlayer
{

	// BeatmapPlayer data

	inline i16 *angleData = nullptr;
	inline u8 *tileData = nullptr;

	inline void set_bpm(float v) { bpm = v; }
	extern void set_event_data(u8 *event_buf, u32 length);
	inline void set_beatmap_data(u8 *angle_buf, u8 *tile_buf, u32 length)
	{
		angleData = reinterpret_cast<i16 *>(angle_buf);
		tileData = tile_buf;
		tileCount = length;
	}

};