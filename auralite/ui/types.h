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

enum class MouseButton { Left, Right, Middle };

struct MouseEvent {
  float x = 0.f;
  float y = 0.f;
  MouseButton button = MouseButton::Left;
  int wheel_delta = 0;
};

struct KeyEvent {
  UINT vk = 0;
  bool down = false;
  bool ctrl = false;
  bool shift = false;
  bool alt = false;
};

}  // namespace auralite::ui
