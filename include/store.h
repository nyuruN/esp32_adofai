#pragma once

#include "rust_typedef.h"
#include "events.h"

#if defined(ESP_PLATFORM)
#include "esp_flash.h"
#endif

#include <vector>
#include <stdlib.h>

struct Beatmap {
    u32 tileCount;
    u32 eventCount;
    u32 bgSize;
    u32 songSize;

    u16* angleData = nullptr; // SPIRAM
    u8* tileData = nullptr; // SPIRAM
    Event* eventData = nullptr; // SPIRAM
	u8* bgData = nullptr; // JPG image data, SPIRAM
	u8* songData = nullptr; // MP3 song data, SPIRAM

	char* songName; // SPIRAM
	float startBpm;

    void* mallocBuf = nullptr;
	u32 totalSize;

    void destroy() {
        if (mallocBuf) free(mallocBuf);
    }
};

// SPIRAM storage for temporary, loadable Beatmaps
class BeatmapStore {
public:
    std::vector<Beatmap> beatmaps;
    u32 current_size = 0; // Current heap utilization
    u32 max_size; // Maximum heap size

    BeatmapStore() {
#if defined(ESP_PLATFORM)
        multi_heap_info_t info;
        heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
        max_size = info.total_free_bytes;
#else
        max_size = -1;
#endif
    }
    void add(Beatmap beatmap) {
        beatmaps.push_back(beatmap);
        current_size += beatmap.totalSize;
    }
    void remove(u32 i) {
        current_size -= beatmaps[i].totalSize;
        beatmaps[i].destroy();
        beatmaps.erase(beatmaps.begin() + i);
    }
    u32 get_available_size() {
        return max_size - current_size;
    }
    void clear() {
        while (beatmaps.size() != 0) {
            beatmaps.back().destroy();
            beatmaps.pop_back();
        }
        current_size = 0;
    };
};

inline BeatmapStore beatmapStore;

// EEPROM (Flash) storage for persistent data
class PersistentStore {
public:
    u32 current_size;
    u32 max_size;


    inline void setCredentials(char *ssid, char *passwd)
    {
    }
    inline void getCredentials(char *ssid, char *passwd)
    {
    }
};