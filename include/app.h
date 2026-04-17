
#include "beatmap.h"
#include "lgfx.h"
#include "data.h"

enum class AppState
{
    StartMenu = 0,
    BeatmapSelection = 1,
    BeatmapPlayer = 2,
    Settings = 3,
    ShowWebsiteLink = 4,
    ShowUploadProgress = 5,
};

class App
{
    AppState state = AppState::BeatmapPlayer;

public:
    void setup()
    {
        BeatmapPlayer::clear();

        // Assign beatmap data
        BeatmapPlayer::set_beatmap_data(Data::angleData, Data::tileData, Data::tileCount);
        BeatmapPlayer::set_bpm(227);
        BeatmapPlayer::set_bpm(20);
        BeatmapPlayer::set_event_data(Data::eventData, Data::eventCount);

        BeatmapPlayer::begin();
    }
    void update(float delta_time)
    {
        switch (state)
        {
        case AppState::BeatmapPlayer:
            BeatmapPlayer::update(delta_time);
            break;
        default:
            break;
        }
    }
    void render(LGFX_Sprite *sprite)
    {
        switch (state)
        {
        case AppState::BeatmapPlayer:
            BeatmapPlayer::render(sprite);
            break;
        default:
            break;
        }
    }
    void press()
    {
    }
    void up()
    {
    }
    void down()
    {
    }
    void left()
    {
    }
    void right()
    {
    }
};