#pragma once

#include "auralite/canvas.h"

namespace auralite::ui {

using RectF = auralite::RectF;
using ColorF = auralite::ColorF;

struct SizeF {
  float w = 0.f;
  float h = 0.f;
};

enum class TextAlign { Left, Center, Right };

// Cross-axis alignment inside Column/Row (main-axis packing stays Start).
enum class Align { Start, Center, End };

// Layout size along one axis:
// Fixed = preferred_* ; Hug = intrinsic content ; Fill = take available max.
enum class SizePolicy { Fixed, Hug, Fill };

enum class MouseButton { Left, Right, Middle };

struct MouseEvent {
  float x = 0.f;
  float y = 0.f;
  MouseButton button = MouseButton::Left;
  int wheel_delta = 0;
  bool shift = false;
  bool ctrl = false;
};

struct KeyEvent {
  UINT vk = 0;
  bool down = false;
  bool ctrl = false;
  bool shift = false;
  bool alt = false;
};

}  // namespace auralite::ui
