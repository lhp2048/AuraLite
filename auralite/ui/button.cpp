#include "auralite/ui/button.h"

#include "auralite/ui/theme.h"
#include "auralite/ui/window.h"

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

Button& Button::variant(ButtonVariant v) {
  variant_ = v;
  bg_.reset();
  bg_hover_.reset();
  bg_pressed_.reset();
  text_color_.reset();
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

Button& Button::text_align(auralite::TextHAlign align) {
  text_align_ = align;
  return *this;
}

Button& Button::corner_radius(float r) {
  corner_radius_ = std::max(0.f, r);
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
  auto variant_fill = [&](bool hover, bool pressed) -> ColorF {
    switch (variant_) {
      case ButtonVariant::Secondary:
        if (pressed) {
          return th.window_bg;
        }
        if (hover) {
          return th.accent_soft;
        }
        return th.surface;
      case ButtonVariant::Danger:
        if (pressed) {
          return th.danger_pressed;
        }
        if (hover) {
          return th.danger_hover;
        }
        return th.danger;
      case ButtonVariant::Primary:
      default:
        if (pressed) {
          return th.accent_pressed;
        }
        if (hover) {
          return th.accent_hover;
        }
        return th.accent;
    }
  };
  if (pressed_) {
    return bg_pressed_.value_or(variant_fill(false, true));
  }
  if (hovered_) {
    return bg_hover_.value_or(variant_fill(true, false));
  }
  return bg_.value_or(variant_fill(false, false));
}

ColorF Button::LabelColor() const {
  const ThemeTokens& th = Theme::Active();
  if (!enabled_) {
    return th.text_muted;
  }
  if (text_color_) {
    return *text_color_;
  }
  // Custom bg (e.g. menu rows) usually sits on a light panel → use body text.
  if (bg_) {
    return th.text;
  }
  if (variant_ == ButtonVariant::Secondary) {
    return th.text;
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

  const float radius = corner_radius_;
  if (radius > 0.f) {
    canvas.FillRoundedRect(bounds_, radius, radius, BgColor());
  } else {
    canvas.FillRect(bounds_, BgColor());
  }
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
                    th.font_ui.c_str(), text_align_);
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

AccRole Button::acc_role() const {
  if (acc_role_override_) {
    return *acc_role_override_;
  }
  if (host_window_ && host_window_->is_popup()) {
    return AccRole::MenuItem;
  }
  return AccRole::Button;
}

std::wstring Button::AccDefaultName() const {
  return text_;
}

AccState Button::acc_state() const {
  AccState s = Node::acc_state();
  s.disabled = !enabled_;
  return s;
}

bool Button::AccInvoke() {
  if (!enabled_) {
    return false;
  }
  if (on_click_) {
    on_click_();
  }
  return true;
}

}  // namespace auralite::ui
