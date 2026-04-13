#include "events.h"
#include "active_events.h"
#include "beatmap.h"

namespace Events
{

	// Traverse events
	void next_floor()
	{
		while (events[p_events].floor < BeatmapPlayer::current_floor && p_events < event_buf_size)
			p_events++;
		/*
		printf("===== Before =====\n");
		printf("pEvents = %d\n", p_events);
		printf("cFloor = %d\n", BeatmapPlayer::current_floor);
		printf("ActiveEvents = %d\n", BeatmapPlayer::current_events);
		printf("BPM = %d\n", BeatmapPlayer::bpm);
		*/

		for (BeatmapPlayer::current_events = 0; events[p_events + BeatmapPlayer::current_events].floor == BeatmapPlayer::current_floor; BeatmapPlayer::current_events++)
		{
			Event *event = &events[p_events + BeatmapPlayer::current_events];

			// Track dispatch status
			dispatched[BeatmapPlayer::current_events] = false;
			if (event->angle_offset != 0)
				continue;

			dispatch_event(event);
			dispatched[BeatmapPlayer::current_events] = true;
		}

		/*
		printf("===== After =====\n");
		printf("pEvents = %d\n", p_events);
		printf("cFloor = %d\n", BeatmapPlayer::current_floor);
		printf("ActiveEvents = %d\n", BeatmapPlayer::current_events);
		printf("BPM = %d\n", BeatmapPlayer::bpm);
		*/
	}
	void update()
	{
		for (u8 i = 0; i < BeatmapPlayer::current_events; i++)
		{
			Event *event = &events[p_events + i];

			if (!dispatched[i] && BeatmapPlayer::angle_progress > event->angle_offset)
			{
				dispatch_event(event);
				dispatched[i] = true;
			}
		}
	}
	void dispatch_event(Event *event)
	{
		switch (event->type)
		{
		case EventType::SetSpeed:
			BeatmapPlayer::bpm = event->set_speed.bpm;
			break;
		case EventType::ShakeScreen:
		case EventType::CameraOffset:
		case EventType::CameraRotation:
		case EventType::CameraZoom:
			ActiveEvents::add_event(ActiveEvents::ActiveEvent{
				.type = event->type,
				.progress = 0,
				.duration = static_cast<u16>((event->shake_screen.duration / 1000.0) / (BeatmapPlayer::bpm / 60.0) * 1000.0),
				.event = event,
			});
			break;
		default:
			break;
		}
	}

}