#include "auralite/ui/slider.h"

#include <algorithm>

namespace auralite::ui {

Slider::Slider() {
  set_focusable(true);
  fill_width();
  fixed_height(28.f);
}

Slider& Slider::value(float v) {
  value_ = std::clamp(v, 0.f, 1.f);
  return *this;
}

Slider& Slider::on_changed(ChangeHandler handler) {
  on_changed_ = std::move(handler);
  return *this;
}

float Slider::TrackLeft() const { return bounds_.x + kThumbR; }

float Slider::TrackWidth() const {
  return std::max(0.f, bounds_.w - kThumbR * 2.f);
}

void Slider::SetValueFromX(float x) {
  const float tw = TrackWidth();
  float v = 0.f;
  if (tw > 0.f) {
    v = (x - TrackLeft()) / tw;
  }
  value_ = std::clamp(v, 0.f, 1.f);
  Notify();
}

void Slider::Notify() {
  if (on_changed_) {
    on_changed_(value_);
  }
}

SizeF Slider::Measure(float max_w, float max_h) {
  return ResolveSize(max_w, max_h,
                     preferred_width() > 0.f ? preferred_width() : max_w, 28.f);
}

void Slider::Paint(auralite::Canvas& canvas) {
  const float cy = bounds_.y + bounds_.h * 0.5f;
  const float track_h = 6.f;
  const RectF track{TrackLeft(), cy - track_h * 0.5f, TrackWidth(), track_h};
  canvas.FillRoundedRect(track, 3.f, 3.f, ColorF::FromRgb(220, 226, 235));
  RectF fill = track;
  fill.w = track.w * value_;
  canvas.FillRoundedRect(fill, 3.f, 3.f, ColorF::FromRgb(40, 110, 200));

  const float tx = TrackLeft() + TrackWidth() * value_;
  const RectF thumb{tx - kThumbR, cy - kThumbR, kThumbR * 2.f, kThumbR * 2.f};
  canvas.FillEllipse(thumb, ColorF::FromRgb(255, 255, 255));
  canvas.DrawEllipse(thumb, focused() ? ColorF::FromRgb(30, 90, 180)
                                      : ColorF::FromRgb(40, 110, 200),
                     2.f);
}

void Slider::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  dragging_ = true;
  SetValueFromX(e.x);
}

void Slider::OnMouseMove(const MouseEvent& e) {
  if (!dragging_) {
    return;
  }
  SetValueFromX(e.x);
}

void Slider::OnMouseUp(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  if (dragging_) {
    SetValueFromX(e.x);
  }
  dragging_ = false;
}

}  // namespace auralite::ui
