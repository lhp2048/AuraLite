#include "auralite/ui/checkbox.h"

#include "auralite/ui/theme.h"

#include <algorithm>

namespace auralite::ui {

Checkbox::Checkbox() {
  set_focusable(true);
}

Checkbox& Checkbox::text(const std::wstring& t) {
  text_ = t;
  return *this;
}

Checkbox& Checkbox::font_size(float size) {
  font_size_ = size;
  return *this;
}

Checkbox& Checkbox::checked(bool v) {
  if (checked_ == v) {
    return *this;
  }
  checked_ = v;
  NotifyAccToggleChanged();
  if (on_changed_) {
    on_changed_(checked_);
  }
  return *this;
}

Checkbox& Checkbox::on_changed(ChangedHandler handler) {
  on_changed_ = std::move(handler);
  return *this;
}

RectF Checkbox::BoxRect() const {
  const float y = bounds_.y + (bounds_.h - kBoxSize) * 0.5f;
  return RectF{bounds_.x, y, kBoxSize, kBoxSize};
}

SizeF Checkbox::Measure(float max_w, float max_h) {
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  const float text_w =
      text_.empty() ? 0.f
                    : auralite::MeasureUiTextWidth(text_, fs, th.font_ui.c_str());
  const float hug_w = kBoxSize + (text_.empty() ? 0.f : kLabelGap + text_w);
  const float hug_h = std::max(kBoxSize, fs + 6.f);
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void Checkbox::Paint(auralite::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  const RectF box = BoxRect();
  canvas.DrawRect(box, th.border, 1.5f);

  if (checked_) {
    const float x0 = box.x + 3.f;
    const float y0 = box.y + 8.f;
    const float x1 = box.x + 7.f;
    const float y1 = box.y + 12.f;
    const float x2 = box.x + 13.f;
    const float y2 = box.y + 4.f;
    canvas.DrawLine(x0, y0, x1, y1, th.glyph, 2.f);
    canvas.DrawLine(x1, y1, x2, y2, th.glyph, 2.f);
  }

  if (!text_.empty()) {
    const float text_x = box.x + kBoxSize + kLabelGap;
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

void Checkbox::Toggle() {
  checked_ = !checked_;
  NotifyAccToggleChanged();
  if (on_changed_) {
    on_changed_(checked_);
  }
}

void Checkbox::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  pressed_ = true;
}

void Checkbox::OnMouseUp(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  const bool was_pressed = pressed_;
  pressed_ = false;
  if (was_pressed && ContainsPoint(bounds_, e.x, e.y)) {
    Toggle();
  }
}

void Checkbox::OnKey(const KeyEvent& e) {
  if (!e.down) {
    return;
  }
  if (e.vk == VK_SPACE || e.vk == VK_RETURN) {
    Toggle();
  }
}

AccRole Checkbox::acc_role() const {
  return AccRole::CheckBox;
}

std::wstring Checkbox::AccDefaultName() const {
  return text_;
}

AccState Checkbox::acc_state() const {
  AccState s = Node::acc_state();
  s.checked = checked_;
  return s;
}

bool Checkbox::AccToggle() {
  Toggle();
  return true;
}

}  // namespace auralite::ui
