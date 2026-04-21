#pragma once

#include "rust_typedef.h"
#include "lgfx.h"
#include "beatmap.h"

// using namespace BeatmapPlayer;

class Tilemap
{
public:
    struct TileDrawData
    {
        float x;
        float y;
    };
    static inline constexpr u32 P_PLAYER_OFFSET = 24;
    static inline constexpr u32 P_TILES = 64;
    u32 p_tiledrawdata = 0; // Represents current_floor
    TileDrawData tiledrawdata[P_TILES] = {};

    // Get n-th tile relative to player position
    TileDrawData &get_relative(i32 idx)
    {
        idx += p_tiledrawdata;
        if (idx < 0)
            idx += P_TILES;
        idx = idx % P_TILES;
        return tiledrawdata[idx];
    }
    // Init tiledrawdata, traverse tilemap relative to current floor
    void init_tiledrawdata()
    {
        tiledrawdata[p_tiledrawdata].x = 0;
        tiledrawdata[p_tiledrawdata].y = 0;
        for (u8 i = 1; i <= (P_TILES - 1 - P_PLAYER_OFFSET); i++)
        {
            if (BeatmapPlayer::current_floor + i >= BeatmapPlayer::tileCount)
                break;
            u8 idx = (p_tiledrawdata + i) % P_TILES;
            tiledrawdata[idx].x = tiledrawdata[(idx - (i8)1) + (1 > idx ? P_TILES : 0)].x + cos(BeatmapPlayer::angleData[BeatmapPlayer::current_floor + i - 1] * 3.14159 / 180) * BeatmapPlayer::beat_radius;
            tiledrawdata[idx].y = tiledrawdata[(idx - (i8)1) + (1 > idx ? P_TILES : 0)].y + sin(BeatmapPlayer::angleData[BeatmapPlayer::current_floor + i - 1] * 3.14159 / 180) * BeatmapPlayer::beat_radius;
        }
        for (u8 i = 1; i <= P_PLAYER_OFFSET; i++)
        {
            if ((int)BeatmapPlayer::current_floor - i < 0)
                break;
            u8 idx = ((i8)p_tiledrawdata - i) + (i > p_tiledrawdata ? P_TILES : 0);
            tiledrawdata[idx].x = tiledrawdata[(idx + 1) % P_TILES].x + cos((BeatmapPlayer::angleData[BeatmapPlayer::current_floor - i] + 180) * 3.14159 / 180) * BeatmapPlayer::beat_radius;
            tiledrawdata[idx].y = tiledrawdata[(idx + 1) % P_TILES].y + sin((BeatmapPlayer::angleData[BeatmapPlayer::current_floor - i] + 180) * 3.14159 / 180) * BeatmapPlayer::beat_radius;
        }
    }
    void next_tile()
    {
        p_tiledrawdata = (p_tiledrawdata + 1) % P_TILES;
        u8 idx = (p_tiledrawdata + (P_TILES - 1 - P_PLAYER_OFFSET)) % P_TILES;
        u8 prev_idx = ((i8)idx - 1) + (1 > idx ? P_TILES : 0);
        tiledrawdata[idx].x = tiledrawdata[prev_idx].x + cos(BeatmapPlayer::angleData[BeatmapPlayer::current_floor + (P_TILES - 2 - P_PLAYER_OFFSET)] * 3.14159 / 180) * BeatmapPlayer::beat_radius;
        tiledrawdata[idx].y = tiledrawdata[prev_idx].y + sin(BeatmapPlayer::angleData[BeatmapPlayer::current_floor + (P_TILES - 2 - P_PLAYER_OFFSET)] * 3.14159 / 180) * BeatmapPlayer::beat_radius;
    }
    void render(LGFX_Sprite *sprite)
    {

        sprite->setTextColor(TFT_BLACK);

        // Draw tiles
        for (i8 i = 0; i < P_TILES; i++)
        {
            const u16 floor_idx = BeatmapPlayer::current_floor + (P_TILES - 1 - P_PLAYER_OFFSET) - i;
            if (floor_idx < 0 || floor_idx >= BeatmapPlayer::tileCount)
                continue; // Clip invalid floors

            i8 idx = (i8)p_tiledrawdata + (P_TILES - 1 - P_PLAYER_OFFSET) - i;
            if (idx < 0)
                idx += P_TILES;
            else
                idx = idx % P_TILES;

            u8 _tile_color = BeatmapPlayer::tile_color;
            u8 border_color = lcd.color332(20, 20, 20);
            const u8 _tileData = BeatmapPlayer::tileData[floor_idx];

            if (_tileData & BeatmapPlayer::TILE_TWIRL)
                border_color = lcd.color332(250, 50, 50);
            if (_tileData & BeatmapPlayer::TILE_CHECKPOINT)
                border_color = lcd.color332(50, 250, 50);
            if (floor_idx <= BeatmapPlayer::current_floor)
                _tile_color = BeatmapPlayer::active_tile_color;
            if (_tileData & BeatmapPlayer::TILE_SPEEDDOWN)
                _tile_color = lcd.color332(80, 80, 200);
            if (_tileData & BeatmapPlayer::TILE_SPEEDUP)
                _tile_color = lcd.color332(200, 80, 80);

            float tile_x = tiledrawdata[idx].x;
            float tile_y = tiledrawdata[idx].y;

            float c = cos(BeatmapPlayer::angleData[floor_idx] * M_PI / 180.0f);
            float s = sin(BeatmapPlayer::angleData[floor_idx] * M_PI / 180.0f);
            float mid_x = tile_x + c * BeatmapPlayer::beat_radius / 2.0f;
            float mid_y = tile_y + s * BeatmapPlayer::beat_radius / 2.0f;
            // b r corner
            float p0x = tile_x + s * BeatmapPlayer::tile_size;
            float p0y = tile_y + -c * BeatmapPlayer::tile_size;
            // b l corner
            float p1x = tile_x - s * BeatmapPlayer::tile_size;
            float p1y = tile_y - -c * BeatmapPlayer::tile_size;
            // t r corner
            float p2x = mid_x + s * BeatmapPlayer::tile_size;
            float p2y = mid_y + -c * BeatmapPlayer::tile_size;
            // t l corner
            float p3x = mid_x - s * BeatmapPlayer::tile_size;
            float p3y = mid_y - -c * BeatmapPlayer::tile_size;

            float pc = cos((BeatmapPlayer::angleData[floor_idx - 1] - 180) * M_PI / 180.0f);
            float ps = sin((BeatmapPlayer::angleData[floor_idx - 1] - 180) * M_PI / 180.0f);
            float pmid_x = tile_x + pc * BeatmapPlayer::beat_radius / 2.0f;
            float pmid_y = tile_y + ps * BeatmapPlayer::beat_radius / 2.0f;
            // b r corner
            float p4x = tile_x + ps * BeatmapPlayer::tile_size;
            float p4y = tile_y + -pc * BeatmapPlayer::tile_size;
            // b l corner
            float p5x = tile_x - ps * BeatmapPlayer::tile_size;
            float p5y = tile_y - -pc * BeatmapPlayer::tile_size;
            // t r corner
            float p6x = pmid_x + ps * BeatmapPlayer::tile_size;
            float p6y = pmid_y + -pc * BeatmapPlayer::tile_size;
            // t l corner
            float p7x = pmid_x - ps * BeatmapPlayer::tile_size;
            float p7y = pmid_y - -pc * BeatmapPlayer::tile_size;

            BeatmapPlayer::camera_transform(&tile_x, &tile_y);
            BeatmapPlayer::camera_transform(&p0x, &p0y);
            BeatmapPlayer::camera_transform(&p1x, &p1y);
            BeatmapPlayer::camera_transform(&p2x, &p2y);
            BeatmapPlayer::camera_transform(&p3x, &p3y);
            BeatmapPlayer::camera_transform(&mid_x, &mid_y);
            BeatmapPlayer::camera_transform(&p4x, &p4y);
            BeatmapPlayer::camera_transform(&p5x, &p5y);
            BeatmapPlayer::camera_transform(&p6x, &p6y);
            BeatmapPlayer::camera_transform(&p7x, &p7y);
            BeatmapPlayer::camera_transform(&pmid_x, &pmid_y);

            sprite->fillCircle(tile_x, tile_y, BeatmapPlayer::tile_size * BeatmapPlayer::DrawData::_zoom, _tile_color);

            sprite->fillTriangle(p0x, p0y, p1x, p1y, p2x, p2y, _tile_color);
            sprite->fillTriangle(p1x, p1y, p2x, p2y, p3x, p3y, _tile_color);
            sprite->fillTriangle(p4x, p4y, p5x, p5y, p6x, p6y, _tile_color);
            sprite->fillTriangle(p5x, p5y, p6x, p6y, p7x, p7y, _tile_color);
            /*
            DrawData::sprite->drawLine(p0x, p0y, p2x, p2y, border_color);
            DrawData::sprite->drawLine(p1x, p1y, p3x, p3y, border_color);
            DrawData::sprite->drawLine(p4x, p4y, p6x, p6y, border_color);
            DrawData::sprite->drawLine(p5x, p5y, p7x, p7y, border_color);
            */

            sprite->drawLine(p2x, p2y, p3x, p3y, border_color);
        }
    }
    void clear()
    {
        p_tiledrawdata = 0;
    }
};

inline Tilemap tilemap;