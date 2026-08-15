#include "auralite/ui/slider.h"

#include "auralite/ui/theme.h"

#include <algorithm>

namespace auralite::ui {

Slider::Slider() {
  set_focusable(true);
  ApplyDefaultSize();
}

void Slider::ApplyDefaultSize() {
  if (IsVertical()) {
    hug_width();
    fill_height();
    fixed_width(tick_count_ >= 2 ? 36.f : 28.f);
  } else {
    fill_width();
    hug_height();
    fixed_height(tick_count_ >= 2 ? 36.f : 28.f);
  }
}

Slider& Slider::value(float v) {
  value_ = std::clamp(v, 0.f, 1.f);
  return *this;
}

Slider& Slider::orientation(SliderOrientation o) {
  orientation_ = o;
  ApplyDefaultSize();
  return *this;
}

Slider& Slider::step(float s) {
  step_ = std::max(0.f, s);
  return *this;
}

Slider& Slider::tick_count(int n) {
  tick_count_ = std::max(0, n);
  ApplyDefaultSize();
  return *this;
}

Slider& Slider::on_changed(ChangeHandler handler) {
  on_changed_ = std::move(handler);
  return *this;
}

float Slider::TrackOrigin() const {
  return IsVertical() ? (bounds_.y + kThumbR) : (bounds_.x + kThumbR);
}

float Slider::TrackLength() const {
  return IsVertical() ? std::max(0.f, bounds_.h - kThumbR * 2.f)
                      : std::max(0.f, bounds_.w - kThumbR * 2.f);
}

void Slider::SetValueFromPointer(float x, float y) {
  const float len = TrackLength();
  float v = 0.f;
  if (len > 0.f) {
    if (IsVertical()) {
      v = 1.f - (y - TrackOrigin()) / len;
    } else {
      v = (x - TrackOrigin()) / len;
    }
  }
  const float prev = value_;
  value_ = std::clamp(v, 0.f, 1.f);
  if (value_ != prev) {
    Notify();
  }
}

void Slider::AdjustValue(float delta) {
  const float prev = value_;
  value_ = std::clamp(value_ + delta, 0.f, 1.f);
  if (value_ != prev) {
    Notify();
  }
}

void Slider::Notify() {
  if (on_changed_) {
    on_changed_(value_);
  }
}

SizeF Slider::Measure(float max_w, float max_h) {
  if (IsVertical()) {
    const float hug_w =
        preferred_width() > 0.f ? preferred_width()
                                : (tick_count_ >= 2 ? 36.f : 28.f);
    return ResolveSize(max_w, max_h, hug_w,
                       preferred_height() > 0.f ? preferred_height() : max_h);
  }
  const float hug_h =
      preferred_height() > 0.f ? preferred_height()
                               : (tick_count_ >= 2 ? 36.f : 28.f);
  return ResolveSize(max_w, max_h,
                     preferred_width() > 0.f ? preferred_width() : max_w, hug_h);
}

void Slider::Paint(auralite::Canvas& canvas) {
  const ThemeTokens& th = Theme::Active();
  const float track_thickness = 6.f;
  RectF track{};
  RectF fill{};
  RectF thumb{};
  const ColorF tick_color = th.text_muted;

  if (IsVertical()) {
    const float cx = bounds_.x + bounds_.w * 0.5f - (tick_count_ >= 2 ? 4.f : 0.f);
    track = RectF{cx - track_thickness * 0.5f, TrackOrigin(), track_thickness,
                  TrackLength()};
    const float thumb_y = TrackOrigin() + TrackLength() * (1.f - value_);
    fill = RectF{track.x, thumb_y, track.w,
                 std::max(0.f, track.y + track.h - thumb_y)};
    thumb = RectF{cx - kThumbR, thumb_y - kThumbR, kThumbR * 2.f, kThumbR * 2.f};

    if (tick_count_ >= 2) {
      for (int i = 0; i < tick_count_; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(tick_count_ - 1);
        const float y = TrackOrigin() + TrackLength() * (1.f - t);
        canvas.DrawLine(cx + track_thickness * 0.5f + 3.f, y,
                        cx + track_thickness * 0.5f + 9.f, y, tick_color, 1.f);
      }
    }
  } else {
    const float cy =
        bounds_.y + bounds_.h * 0.5f - (tick_count_ >= 2 ? 4.f : 0.f);
    track = RectF{TrackOrigin(), cy - track_thickness * 0.5f, TrackLength(),
                  track_thickness};
    fill = track;
    fill.w = track.w * value_;
    const float tx = TrackOrigin() + TrackLength() * value_;
    thumb = RectF{tx - kThumbR, cy - kThumbR, kThumbR * 2.f, kThumbR * 2.f};

    if (tick_count_ >= 2) {
      for (int i = 0; i < tick_count_; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(tick_count_ - 1);
        const float x = TrackOrigin() + TrackLength() * t;
        canvas.DrawLine(x, cy + track_thickness * 0.5f + 3.f, x,
                        cy + track_thickness * 0.5f + 9.f, tick_color, 1.f);
      }
    }
  }

  canvas.FillRoundedRect(track, 3.f, 3.f, th.scroll_track);
  canvas.FillRoundedRect(fill, 3.f, 3.f, th.accent);
  canvas.FillEllipse(thumb, th.surface);
  canvas.DrawEllipse(thumb, focused() ? th.border_focus : th.accent, 2.f);
}

void Slider::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  dragging_ = true;
  SetValueFromPointer(e.x, e.y);
}

void Slider::OnMouseMove(const MouseEvent& e) {
  if (!dragging_) {
    return;
  }
  SetValueFromPointer(e.x, e.y);
}

void Slider::OnMouseUp(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  if (dragging_) {
    SetValueFromPointer(e.x, e.y);
  }
  dragging_ = false;
}

void Slider::OnKey(const KeyEvent& e) {
  if (!e.down) {
    return;
  }
  const float s = step_ > 0.f ? step_ : 0.05f;
  if (IsVertical()) {
    if (e.vk == VK_UP || e.vk == VK_PRIOR) {
      AdjustValue(e.vk == VK_PRIOR ? s * 5.f : s);
    } else if (e.vk == VK_DOWN || e.vk == VK_NEXT) {
      AdjustValue(e.vk == VK_NEXT ? -s * 5.f : -s);
    } else if (e.vk == VK_HOME) {
      AdjustValue(1.f - value_);
    } else if (e.vk == VK_END) {
      AdjustValue(-value_);
    }
  } else {
    if (e.vk == VK_RIGHT || e.vk == VK_NEXT) {
      AdjustValue(e.vk == VK_NEXT ? s * 5.f : s);
    } else if (e.vk == VK_LEFT || e.vk == VK_PRIOR) {
      AdjustValue(e.vk == VK_PRIOR ? -s * 5.f : -s);
    } else if (e.vk == VK_HOME) {
      AdjustValue(-value_);
    } else if (e.vk == VK_END) {
      AdjustValue(1.f - value_);
    }
  }
}

}  // namespace auralite::ui
