
#include "rust_typedef.h"

#if defined(ESP_PLATFORM)

#include <AudioOutputI2S.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioFileSourceLittleFS.h>

#include <AudioGeneratorOpus.h>

// Opus is decode heavy, but for the memory limitation
// it really is the best bet
//#include <AudioGeneratorMP3.h>
//typedef AudioGeneratorMP3 AudioGeneratorOpus;

class MusicPlayer {
    AudioGeneratorOpus opus;
    AudioFileSourcePROGMEM mem_file;
    AudioFileSourceLittleFS fs_file;
    AudioOutputI2S out;
    AudioFileSource* file;
    bool in_memory;

public:

    void setup() {
        mem_file = AudioFileSourcePROGMEM();
        fs_file = AudioFileSourceLittleFS();
        out = AudioOutputI2S();
        opus = AudioGeneratorOpus();
        file = nullptr;

        out.SetPinout(12, 11, 14);
        out.SetGain(0.05);
    }
    // For beatmaps
    void set_song(const void* ptr, u32 len) {
        if (mem_file.isOpen()) mem_file.close();
        printf("File at %p with size %d\n", ptr, len);
        mem_file.open(ptr, len);
        file = &mem_file;
    }
    // For menu bgm and other static resources
    void set_song(const char* filename) {
        if (fs_file.isOpen()) fs_file.close();
        fs_file.open(filename);
        file = &fs_file;
    }
    void play() {
        if (opus.isRunning()) opus.stop();
        if (!file) return;
        printf("Begin\n");
        opus.begin(file, &out);
        printf("After\n");
    }
    void update() {
        if (opus.isRunning())
        {
            if (!opus.loop())
            {
                opus.stop();
            }
        }
    }
};

#else

class MusicPlayer {
public:
    void setup() {}
    void set_song(const void* ptr, u32 len) {}
    void set_song(const char* filename) {}
    void play() {}
    void update() {}
};
#endif 

inline MusicPlayer music_player;