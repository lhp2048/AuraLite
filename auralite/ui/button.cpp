#include "auralite/ui/button.h"

#include "auralite/ui/theme.h"

#include <algorithm>

namespace auralite::ui {

Button::Button() {
  set_focusable(true);
  // Form default: stretch horizontally. In toolbars/Rows prefer width: hug.
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

Button& Button::bg(const ColorF& c) {
  bg_ = c;
  return *this;
}

Button& Button::bg_hover(const ColorF& c) {
  bg_hover_ = c;
  return *this;
}

Button& Button::bg_pressed(const ColorF& c) {
  bg_pressed_ = c;
  return *this;
}

Button& Button::text_color(const ColorF& c) {
  text_color_ = c;
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

Button& Button::set_enabled(bool e) {
  enabled_ = e;
  if (!enabled_) {
    hovered_ = false;
    pressed_ = false;
  }
  return *this;
}

SizeF Button::Measure(float max_w, float max_h) {
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  const float hug_w =
      text_.empty()
          ? preferred_width()
          : (auralite::MeasureUiTextWidth(text_, fs, th.font_ui.c_str()) +
             24.f);
  const float hug_h = preferred_height() > 0.f ? preferred_height() : 40.f;
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

ColorF Button::BgColor() const {
  const ThemeTokens& th = Theme::Active();
  if (!enabled_) {
    return th.surface_alt;
  }
  if (pressed_) {
    return bg_pressed_.value_or(th.accent_pressed);
  }
  if (hovered_) {
    return bg_hover_.value_or(th.accent_hover);
  }
  return bg_.value_or(th.accent);
}

ColorF Button::LabelColor() const {
  const ThemeTokens& th = Theme::Active();
  if (!enabled_) {
    return th.text_muted;
  }
  if (text_color_) {
    return *text_color_;
  }
  return th.text_on_accent;
}

void Button::EnsureIcon(auralite::Canvas& canvas) {
  if (!icon_.empty() || icon_pixels_.empty() || icon_w_ == 0 || icon_h_ == 0) {
    return;
  }
  icon_.CreateFromBgra(canvas, icon_w_, icon_h_, icon_pixels_.data(),
                       icon_stride_);
}

void Button::Paint(auralite::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  EnsureIcon(canvas);
  const ThemeTokens& th = Theme::Active();

  const float radius = 8.f;
  canvas.FillRoundedRect(bounds_, radius, radius, BgColor());
  if (focused()) {
    // Dashed focus ring — less heavy than a solid overlay on filled buttons.
    canvas.DrawDashedRect(bounds_, th.border_focus, 1.f);
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
    canvas.DrawText(text_, text_rect, LabelColor(), ResolveFontSize(font_size_),
                    th.font_ui.c_str(), auralite::TextHAlign::Center);
  }
}

void Button::OnMouseDown(const MouseEvent& e) {
  if (!enabled_ || e.button != MouseButton::Left) {
    return;
  }
  pressed_ = true;
}

void Button::OnMouseUp(const MouseEvent& e) {
  if (!enabled_ || e.button != MouseButton::Left) {
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
  if (!enabled_ || !e.down) {
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
