#include "auralite/ui/switch_control.h"

#include <algorithm>

namespace auralite::ui {

Switch::Switch() {
  set_focusable(true);
}

Switch& Switch::text(const std::wstring& t) {
  text_ = t;
  return *this;
}

Switch& Switch::font_size(float size) {
  font_size_ = size;
  return *this;
}

Switch& Switch::on(bool v) {
  if (on_ == v) {
    return *this;
  }
  on_ = v;
  if (on_changed_) {
    on_changed_(on_);
  }
  return *this;
}

Switch& Switch::on_changed(ChangedHandler handler) {
  on_changed_ = std::move(handler);
  return *this;
}

RectF Switch::TrackRect() const {
  const float y = bounds_.y + (bounds_.h - kTrackHeight) * 0.5f;
  return RectF{bounds_.x, y, kTrackWidth, kTrackHeight};
}

SizeF Switch::Measure(float max_w, float max_h) {
  const float text_w =
      text_.empty() ? 0.f
                    : auralite::MeasureUiTextWidth(text_, font_size_);
  const float hug_w =
      kTrackWidth + (text_.empty() ? 0.f : kLabelGap + text_w);
  const float hug_h = std::max(kTrackHeight, font_size_ + 6.f);
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void Switch::Paint(auralite::Canvas& canvas) {
  const RectF track = TrackRect();
  const ColorF track_color =
      on_ ? ColorF::FromRgb(40, 110, 200) : ColorF::FromRgb(180, 180, 180);
  const float radius = kTrackHeight * 0.5f;
  canvas.FillRoundedRect(track, radius, radius, track_color);

  const float thumb_y = track.y + (kTrackHeight - kThumbSize) * 0.5f;
  const float thumb_x =
      on_ ? (track.x + kTrackWidth - kThumbSize - 2.f) : (track.x + 2.f);
  const RectF thumb{thumb_x, thumb_y, kThumbSize, kThumbSize};
  canvas.FillEllipse(thumb, ColorF::FromRgb(255, 255, 255));
  canvas.DrawEllipse(thumb, ColorF::FromRgb(120, 120, 120), 1.f);

  if (!text_.empty()) {
    const float text_x = track.x + kTrackWidth + kLabelGap;
    const RectF text_rect{text_x, bounds_.y,
                          std::max(0.f, bounds_.x + bounds_.w - text_x),
                          bounds_.h};
    canvas.DrawText(text_, text_rect, ColorF::FromRgb(30, 40, 55), font_size_,
                    L"Microsoft YaHei UI", auralite::TextHAlign::Left);
  }

  if (focused()) {
    canvas.DrawRect(bounds_, ColorF::FromRgb(40, 110, 200), 1.5f);
  }
}

void Switch::Toggle() {
  on_ = !on_;
  if (on_changed_) {
    on_changed_(on_);
  }
}

void Switch::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  pressed_ = true;
}

void Switch::OnMouseUp(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  const bool was_pressed = pressed_;
  pressed_ = false;
  if (was_pressed && ContainsPoint(bounds_, e.x, e.y)) {
    Toggle();
  }
}

void Switch::OnKey(const KeyEvent& e) {
  if (!e.down) {
    return;
  }
  if (e.vk == VK_SPACE || e.vk == VK_RETURN) {
    Toggle();
  }
}

}  // namespace auralite::ui
