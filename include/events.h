#pragma once

#include "rust_typedef.h"

enum class EventType : u8
{
  CameraOffset = 0,
  CameraZoom = 1,
  CameraRotation = 2,
  ShakeScreen = 3,
  SetSpeed = 4,
};
enum class EaseType : u8
{
  Linear = 0,
  EaseIn = 1,
  EaseOut = 2,
  EaseInOut = 3,
  InCirc = 4,
  OutCirc = 5,
};

// 7 bytes
struct ShakeScreen
{
  u16 duration;  // in beats = 180 deg (inherits current bpm)
  u16 intensity; // in 1/100 frequency
  u16 strength;  // in 1/100 tile size
  EaseType ease;
} __attribute__((packed));
// 7 bytes
struct CameraRotation
{
  u16 duration;   // in mili beats
  float rotation; // in degrees
  EaseType ease;
} __attribute__((packed));
// 7 bytes = 2 + 2 + 2 + 2
struct CameraOffset
{
  u16 duration; // in mili beats
  i16 offset_x; // in mili tile size
  i16 offset_y; // in mili tile size
  EaseType ease;
} __attribute__((packed));
// 7 bytes
struct CameraZoom
{
  u16 duration; // in mili beats
  float zoom;   // in percent
  EaseType ease;
} __attribute__((packed));
// 4 bytes
struct SetSpeed
{
  float bpm; // in beats per minute
} __attribute__((packed));
// 1 + 4 + 2 + 7 (union) = 14 bytes
struct Event
{
  EventType type;
  u32 floor;
  u16 angle_offset;
  union
  {
    CameraOffset camera_offset;
    CameraRotation camera_rotation;
    CameraZoom camera_zoom;
    ShakeScreen shake_screen;
    SetSpeed set_speed;
  };
} __attribute__((packed));

class BeatmapEvents
{
public:
  struct ActiveEvent
  {
    EventType type;
    u16 progress; // in ms
    u16 duration; // in ms
    Event *event;
  };

  static inline constexpr u32 MAX_EVENT_COUNT = 16;

  ActiveEvent active_event_buf[32] = {};
  u8 active_count = 0;

  u32 p_events;                  // Pointer to first event
  u32 current_events;            // Number of events on the current floor starting from p_events
  bool dispatched[32] = {false}; // State of dispatch of events starting from p_events

  // Data
  u32 event_buf_size = 0;
  Event *events = nullptr;

  // Traverse events
  void next_floor();
  // Dispatch undispatched events
  void update(u8 delta_millis);
  void dispatch_event(Event *event);
  void clear()
  {
    p_events = 0;
    active_count = 0;
  }

  // TODO: handle overflow
  void add_event(ActiveEvent event)
  {
		if (event.duration == 0) {
			apply(&event);
			return;
		}
    active_event_buf[active_count] = event;
    active_count++;
  }
  /// Applies event values permanently
  void apply(ActiveEvent *event);
  /// Applies event values (transition)
  void apply(float *camera_x, float *camera_y, float *rotation, float *zoom);
  // Remove and apply finished events
  inline void cull_active_events();
};

inline BeatmapEvents beatmap_events;