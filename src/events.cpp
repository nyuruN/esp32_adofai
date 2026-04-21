#include "events.h"
#include "beatmap.h"
#include "tilemap.h"
#include "ease.h"

void BeatmapEvents::next_floor()
{
	while (events[p_events].floor < BeatmapPlayer::current_floor && p_events < event_buf_size)
		p_events++;

	for (current_events = 0; events[p_events + current_events].floor == BeatmapPlayer::current_floor; current_events++)
	{
		Event *event = &events[p_events + current_events];

		// Track dispatch status
		dispatched[current_events] = false;
		if (event->angle_offset != 0)
			continue;

		dispatch_event(event);
		dispatched[current_events] = true;
	}
}
// Dispatch undispatched events
void BeatmapEvents::update(u8 delta_millis)
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

	for (u8 i = 0; i < active_count; i++)
	{
		active_event_buf[i].progress += delta_millis;
	}

	cull_active_events();
}
void BeatmapEvents::dispatch_event(Event *event)
{
	switch (event->type)
	{
	case EventType::SetSpeed:
		BeatmapPlayer::bpm = event->set_speed.bpm;
		break;
	case EventType::CameraSetMode:
		printf("Set mode %d\n", event->camera_set_mode.relative_to);
		if (event->camera_set_mode.relative_to == RelativeTo::Player) {
			BeatmapPlayer::camera_mode = RelativeTo::Player;
			break;
		} else { // Begin transition to Tile
			BeatmapPlayer::camera_mode = RelativeTo::Tile;
			BeatmapPlayer::prev_anchor_x = BeatmapPlayer::camera_x;
			BeatmapPlayer::prev_anchor_y = BeatmapPlayer::camera_y;
			BeatmapPlayer::anchor_x = tilemap.get_relative(0).x;
			BeatmapPlayer::anchor_y = tilemap.get_relative(0).y;
			BeatmapPlayer::transition = 0.0f;
		}
	case EventType::ShakeScreen:
	case EventType::CameraOffset:
	case EventType::CameraRotation:
	case EventType::CameraZoom:
		add_event(ActiveEvent{
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

/// Applies event values permanently
void BeatmapEvents::apply(ActiveEvent *event)
{
	const Event *e = event->event;

	switch (event->type)
	{
	case EventType::CameraZoom:
		BeatmapPlayer::zoom = e->camera_zoom.zoom;
		break;
	case EventType::CameraRotation:
		BeatmapPlayer::rotation = e->camera_rotation.rotation;
		break;
	case EventType::CameraOffset:
		BeatmapPlayer::offset_x = e->camera_offset.offset_x / 1000.0f * BeatmapPlayer::beat_radius / 1.5;
		BeatmapPlayer::offset_y = e->camera_offset.offset_y / 1000.0f * BeatmapPlayer::beat_radius / 1.5;
		break;
	case EventType::CameraSetMode:
		break;
	default:
		break;
	}
}
/// Applies event values (transition)
void BeatmapEvents::apply(float *camera_x, float *camera_y, float *rotation, float *zoom)
{
	for (u8 i = 0; i < active_count; i++)
	{
		const ActiveEvent *event = &active_event_buf[i];
		const Event *e = event->event;
		const float p = (float)event->progress / (float)event->duration;
		const float t = lgfx::millis() / 1000.0f;

		if (event->progress >= event->duration)
			continue;

		switch (event->type)
		{
		case EventType::ShakeScreen:
			*camera_x += sin(t * e->shake_screen.intensity) * (float)BeatmapPlayer::beat_radius * ((float)e->shake_screen.strength / 450.0f);
			break;
		case EventType::CameraZoom:
			*zoom += (e->camera_zoom.zoom - *zoom) * p;
			break;
		case EventType::CameraRotation:
			*rotation += (e->camera_rotation.rotation - *rotation) * p;
			break;
		case EventType::CameraOffset:
			*camera_x += (e->camera_offset.offset_x / 1000.0f) * BeatmapPlayer::beat_radius * p / 1.5;
			*camera_y += (e->camera_offset.offset_y / 1000.0f) * BeatmapPlayer::beat_radius * p / 1.5;
			break;
		case EventType::CameraSetMode:
			// Set transition value for Tile mode
			BeatmapPlayer::transition = easeOutBack(p);
			break;
		default:
			break;
		}
	}
}
// Remove and apply finished events
void BeatmapEvents::cull_active_events()
{
	if (active_count == 0)
		return;

	// ri is the index that should be removed
	u8 ri;
	for (u8 i = 0; i < active_count; i++)
	{
		if (active_event_buf[i].progress >= active_event_buf[i].duration)
		{
			ri = i;
			break;
		}
		else if (i == active_count - 1)
		{ // All clean
			return;
		}
	}
	apply(&active_event_buf[ri]);
	// Special case, ri is the last element
	if (ri == active_count - 1)
	{
		active_count--;
		return;
	}
	// si repeatedly scans for the next valid event and ...
	for (u8 si = ri + 1; si < active_count; si++)
	{
		// ... moves valid event to to ri
		if (active_event_buf[si].progress < active_event_buf[si].duration)
		{
			active_event_buf[ri] = active_event_buf[si];
			ri++;
			active_count--;
		}
		else
		{
			apply(&active_event_buf[si]);
		}
	}
}