#pragma once

#include "events.h"
#include "beatmap.h"

using namespace Events;

namespace ActiveEvents
{
  // Types

  struct ActiveEvent
  {
    EventType type;
    u16 progress; // in ms
    u16 duration; // in ms
    Event *event;
  };

  // Variables

  inline ActiveEvent event_buf[16] = {};
  inline u8 count = 0;

  // Functions

  // TODO: handle overflow
  inline void add_event(ActiveEvent event)
  {
    event_buf[count] = event;
    count++;
  }
  /// Applies event values permanently
  inline void apply(ActiveEvent *event)
  {
    const Event *e = event->event;

    switch (event->type)
    {
    case EventType::CameraZoom:
      BeatmapPlayer::zoom = e->camera_zoom.zoom / 1000.0f;
      break;
    case EventType::CameraRotation:
      BeatmapPlayer::rotation = e->camera_rotation.rotation;
      break;
    case EventType::CameraOffset:
      BeatmapPlayer::offset_x = e->camera_offset.offset_x / 1300.0f * BeatmapPlayer::beat_radius;
      BeatmapPlayer::offset_y = e->camera_offset.offset_y / 1300.0f * BeatmapPlayer::beat_radius;
    default:
      break;
    }
  }
  /// Applies event values (transition)
  inline void apply(float *camera_x, float *camera_y, float *rotation, float *zoom)
  {
    for (u8 i = 0; i < count; i++)
    {
      const ActiveEvent *event = &event_buf[i];
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
        *zoom += (e->camera_zoom.zoom / 1000.0f - *zoom) * p;
        break;
      case EventType::CameraRotation:
        *rotation += (e->camera_rotation.rotation / 1000.0f - *rotation) * p;
        break;
      case EventType::CameraOffset:
        *camera_x += (e->camera_offset.offset_x / 1300.0f) * BeatmapPlayer::beat_radius * p;
        *camera_y += (e->camera_offset.offset_y / 1300.0f) * BeatmapPlayer::beat_radius * p;
      default:
        break;
      }
    }
  }
  /// Culls invalid events
  inline void clean()
  {
    if (count == 0)
      return;

    // ri is the index that should be removed
    u8 ri;
    for (u8 i = 0; i < count; i++)
    {
      if (event_buf[i].progress >= event_buf[i].duration)
      {
        ri = i;
        break;
      }
      else if (i == count - 1)
      { // All clean;
        return;
      }
    }
    apply(&event_buf[ri]);
    // Special case, ri is the last element
    if (ri == count - 1)
    {
      count--;
      return;
    }
    // si repeatedly scans for the next valid event and ...
    for (u8 si = ri + 1; si < count; si++)
    {
      // ... moves valid event to to ri
      if (event_buf[si].progress < event_buf[si].duration)
      {
        event_buf[ri] = event_buf[si];
        ri++;
        count--;
      }
      else
      {
        apply(&event_buf[si]);
      }
    }
  }
  /// Tick events
  inline void update(u8 delta_millis)
  {
    for (u8 i = 0; i < count; i++)
    {
      event_buf[i].progress += delta_millis;
    }
    clean();
  }

};