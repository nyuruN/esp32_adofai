
#include "beatmap.h"
#include "lgfx.h"
#include "data.h"
#include "networkservice.h"

enum class AppState
{
    MainMenu = 0,
    BeatmapSelection = 1,
    BeatmapPlayer = 2,
    Settings = 3,
    ShowWebsiteLink = 4,
    ShowUploadProgress = 5,
};
enum class Input
{
    Press,
    Up,
    Left,
    Right,
    Down,
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

        setup_webserver();
    }
    void update(float delta_time)
    {
        network_update();
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
        case AppState::MainMenu:
            sprite->clear();
            sprite->setCursor(80, 100);
            sprite->printf("Click to play");
            break;
        default:
            break;
        }
    }
    void input(Input input) {
        switch (state)
        {
        case AppState::BeatmapPlayer:
            if (input == Input::Press)
                BeatmapPlayer::hit();
            break;
        case AppState::MainMenu:
            if (input == Input::Press) {
                BeatmapPlayer::begin();
                state = AppState::BeatmapPlayer;
            }
            break;
        default:
            break;
        }
    }
    void press()
    {
    }
};