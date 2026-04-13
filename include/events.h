
#pragma once
#include "rust_typedef.h"

namespace Events
{
  // Types

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

  // Variables

  inline u16 p_events; // Event traversal tracker
  inline bool dispatched[16] = {false};

  // Functions

  extern void next_floor();
  extern void update();
  extern void dispatch_event(Event *event);

  // Data
  inline u32 event_buf_size = 0;
  inline Event *events = nullptr;

}
