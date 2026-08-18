#include "auralite/ui/switch_control.h"

#include "auralite/ui/theme.h"
#include "auralite/ui/window.h"

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
  SyncThumb(false);
  NotifyAccToggleChanged();
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
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  const float text_w =
      text_.empty() ? 0.f
                    : auralite::MeasureUiTextWidth(text_, fs, th.font_ui.c_str());
  const float hug_w =
      kTrackWidth + (text_.empty() ? 0.f : kLabelGap + text_w);
  const float hug_h = std::max(kTrackHeight, fs + 6.f);
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void Switch::Paint(auralite::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  const RectF track = TrackRect();
  const float t = std::clamp(thumb_t_, 0.f, 1.f);
  const ColorF track_color{
      th.border.r + (th.accent.r - th.border.r) * t,
      th.border.g + (th.accent.g - th.border.g) * t,
      th.border.b + (th.accent.b - th.border.b) * t,
      th.border.a + (th.accent.a - th.border.a) * t,
  };
  const float radius = kTrackHeight * 0.5f;
  canvas.FillRoundedRect(track, radius, radius, track_color);

  const float thumb_y = track.y + (kTrackHeight - kThumbSize) * 0.5f;
  const float x0 = track.x + 2.f;
  const float x1 = track.x + kTrackWidth - kThumbSize - 2.f;
  const float thumb_x = x0 + (x1 - x0) * t;
  const RectF thumb{thumb_x, thumb_y, kThumbSize, kThumbSize};
  canvas.FillEllipse(thumb, th.surface);
  canvas.DrawEllipse(thumb, th.border, 1.f);

  if (!text_.empty()) {
    const float text_x = track.x + kTrackWidth + kLabelGap;
    const RectF text_rect{text_x, bounds_.y,
                          std::max(0.f, bounds_.x + bounds_.w - text_x),
                          bounds_.h};
    canvas.DrawText(text_, text_rect, th.text, ResolveFontSize(font_size_),
                    th.font_ui.c_str(), auralite::TextHAlign::Left);
  }

  if (focused()) {
    canvas.DrawDashedRect(bounds_, th.border_focus, 1.f);
  }
}

void Switch::Toggle() {
  on_ = !on_;
  SyncThumb(false);
  NotifyAccToggleChanged();
  if (on_changed_) {
    on_changed_(on_);
  }
}

void Switch::SyncThumb(bool instant) {
  const float to = on_ ? 1.f : 0.f;
  if (instant || !CanTween()) {
    thumb_tween_.Cancel();
    thumb_t_ = to;
    return;
  }
  const float from = thumb_t_;
  if (from == to) {
    return;
  }
  thumb_tween_.Start(
      host_window(), kUiAnimSec, Easing::EaseOutCubic,
      [this, from, to](float t) { thumb_t_ = from + (to - from) * t; },
      [this, to] { thumb_t_ = to; });
}

void Switch::OnAnimateChanged() { SyncThumb(true); }

void Switch::OnHostWindowChanged() {
  if (!CanTween()) {
    SyncThumb(true);
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

AccRole Switch::acc_role() const {
  return AccRole::CheckBox;
}

std::wstring Switch::AccDefaultName() const {
  return text_;
}

AccState Switch::acc_state() const {
  AccState s = Node::acc_state();
  s.checked = on_;
  return s;
}

bool Switch::AccToggle() {
  Toggle();
  return true;
}

}  // namespace auralite::ui
