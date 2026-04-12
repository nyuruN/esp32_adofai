#include "events.h"
#include "camera_events.h"
#include "beatmap.h"

namespace Events {

// Traverse events
void next_floor() {
	while (events[p_events].floor < Beatmap::current_floor && p_events < event_buf_size) p_events++;
	/*
	printf("===== Before =====\n");
	printf("pEvents = %d\n", p_events);
	printf("cFloor = %d\n", Beatmap::current_floor);
	printf("ActiveEvents = %d\n", Beatmap::current_events);
	printf("BPM = %d\n", Beatmap::bpm);
	*/

	for (Beatmap::current_events = 0; events[p_events + Beatmap::current_events].floor == Beatmap::current_floor; Beatmap::current_events++) {
		Event* event = &events[p_events + Beatmap::current_events];

		// Track dispatch status
		dispatched[Beatmap::current_events] = false;
		if (event->angle_offset != 0) continue;

		dispatch_event(event);
		dispatched[Beatmap::current_events] = true;
	}

	/*
	printf("===== After =====\n");
	printf("pEvents = %d\n", p_events);
	printf("cFloor = %d\n", Beatmap::current_floor);
	printf("ActiveEvents = %d\n", Beatmap::current_events);
	printf("BPM = %d\n", Beatmap::bpm);
	*/
}
void update() {
	for (u8 i = 0; i < Beatmap::current_events; i++) {
		Event* event = &events[p_events + i];

		if (!dispatched[i] && Beatmap::angle_progress > event->angle_offset) {
			dispatch_event(event);
			dispatched[i] = true;
		}
	}

}
void dispatch_event(Event* event) {
	switch (event->type)
	{
	case EventType::SetSpeed:
		Beatmap::bpm = event->set_speed.bpm;
		break;
	case EventType::ShakeScreen:
	case EventType::CameraOffset:
	case EventType::CameraRotation:
	case EventType::CameraZoom:
		CameraEvents::add_event(CameraEvents::CameraEvent {
		.type = event->type,
		.progress = 0,
		.duration = static_cast<u16>((event->shake_screen.duration / 1000.0) / (Beatmap::bpm / 60.0) * 1000.0),
		.event = event,
		});
		break;
	default:
		break;
	}
}

}