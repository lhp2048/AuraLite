#pragma once

#include "auralite/canvas.h"

#include <string>

namespace auralite::ui {

using RectF = auralite::RectF;
using ColorF = auralite::ColorF;

struct SizeF {
  float w = 0.f;
  float h = 0.f;
};

enum class TextAlign { Left, Center, Right };

// Single-line Label overflow. Ignored when Label::wrap() is true.
enum class TextTrim { Clip, Start, Middle, End };

// Placement along one screen axis (not Label text align).
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

// Window shortcut: Ctrl/Alt chord, F1–F24, or Esc. Bare letters are rejected.
struct KeyChord {
  UINT vk = 0;
  bool ctrl = false;
  bool alt = false;
  bool shift = false;

  bool operator==(const KeyChord& o) const {
    return vk == o.vk && ctrl == o.ctrl && alt == o.alt && shift == o.shift;
  }

  bool Matches(const KeyEvent& e) const {
    return e.down && e.vk == vk && e.ctrl == ctrl && e.alt == alt &&
           e.shift == shift;
  }

  bool IsShortcut() const {
    if (vk == 0) {
      return false;
    }
    if (ctrl || alt) {
      return true;
    }
    if (vk >= VK_F1 && vk <= VK_F24) {
      return true;
    }
    return vk == VK_ESCAPE;
  }
};

// "Ctrl+S", "Alt+Shift+O", "F1", "Esc". Case-insensitive. Returns false if
// empty, unknown token, or not IsShortcut().
bool ParseKeyChord(const std::string& spec, KeyChord* out);

}  // namespace auralite::ui
