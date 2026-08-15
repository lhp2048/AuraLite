#include "auralite/ui/button.h"

#include <algorithm>

namespace auralite::ui {

Button::Button() {
  set_focusable(true);
  // Typical form control: stretch horizontally, keep a fixed control height.
  fill_width();
  fixed_height(40.f);
  set_preferred_width(140.f);
}

Button& Button::text(const std::wstring& t) {
  text_ = t;
  return *this;
}

Button& Button::font_size(float size) {
  font_size_ = size;
  return *this;
}

Button& Button::on_click(ClickHandler handler) {
  on_click_ = std::move(handler);
  return *this;
}

Button& Button::preferred_size(float w, float h) {
  fixed_width(w);
  fixed_height(h);
  return *this;
}

Button& Button::icon_bgra(UINT width, UINT height, const uint8_t* bgra,
                          UINT stride) {
  icon_.Reset();
  icon_w_ = width;
  icon_h_ = height;
  icon_stride_ = stride;
  icon_pixels_.assign(bgra, bgra + stride * height);
  return *this;
}

SizeF Button::Measure(float max_w, float max_h) {
  const float hug_w =
      text_.empty() ? preferred_width()
                    : (auralite::MeasureUiTextWidth(text_, font_size_) + 24.f);
  const float hug_h = preferred_height() > 0.f ? preferred_height() : 40.f;
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

ColorF Button::BgColor() const {
  if (pressed_) {
    return ColorF::FromRgb(25, 85, 160);
  }
  if (hovered_) {
    return ColorF::FromRgb(55, 130, 215);
  }
  return ColorF::FromRgb(40, 110, 200);
}

void Button::EnsureIcon(auralite::Canvas& canvas) {
  if (!icon_.empty() || icon_pixels_.empty() || icon_w_ == 0 || icon_h_ == 0) {
    return;
  }
  icon_.CreateFromBgra(canvas, icon_w_, icon_h_, icon_pixels_.data(),
                       icon_stride_);
}

void Button::Paint(auralite::Canvas& canvas) {
  EnsureIcon(canvas);

  const float radius = 8.f;
  canvas.FillRoundedRect(bounds_, radius, radius, BgColor());
  if (focused()) {
    canvas.DrawRect(bounds_, ColorF::FromRgb(20, 60, 120), 2.f);
  }

  float text_x = bounds_.x + 12.f;

  if (!icon_.empty()) {
    const float icon_side = std::min(bounds_.h - 12.f, 24.f);
    const RectF icon_rect{bounds_.x + 12.f,
                          bounds_.y + (bounds_.h - icon_side) * 0.5f, icon_side,
                          icon_side};
    canvas.DrawImage(icon_, icon_rect);
    text_x = icon_rect.x + icon_rect.w + 8.f;
  }

  if (!text_.empty()) {
    const RectF text_rect{
        text_x, bounds_.y,
        std::max(0.f, bounds_.x + bounds_.w - 12.f - text_x), bounds_.h};
    canvas.DrawText(text_, text_rect, ColorF::FromRgb(255, 255, 255), font_size_,
                    L"Microsoft YaHei UI", auralite::TextHAlign::Center);
  }
}

void Button::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  pressed_ = true;
}

void Button::OnMouseUp(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  const bool was_pressed = pressed_;
  pressed_ = false;
  if (was_pressed && ContainsPoint(bounds_, e.x, e.y) && on_click_) {
    on_click_();
  }
}

void Button::OnMouseEnter(const MouseEvent&) {
  hovered_ = true;
}

void Button::OnMouseLeave(const MouseEvent&) {
  hovered_ = false;
  pressed_ = false;
}

void Button::OnKey(const KeyEvent& e) {
  if (!e.down) {
    return;
  }
  if ((e.vk == VK_SPACE || e.vk == VK_RETURN) && on_click_) {
    on_click_();
  }
}

void Button::OnDeviceLost() {
  icon_.Reset();
  Node::OnDeviceLost();
}

}  // namespace auralite::ui
