#include "auralite/ui/toast.h"

#include "auralite/canvas.h"
#include "auralite/ui/theme.h"

#include <algorithm>

namespace auralite::ui {

Toast::Toast() {
  hug_width();
  hug_height();
}

Toast& Toast::text(const std::wstring& t) {
  text_ = t;
  return *this;
}

Toast& Toast::variant(ToastVariant v) {
  variant_ = v;
  return *this;
}

Toast& Toast::duration_sec(float s) {
  duration_sec_ = s;
  return *this;
}

Toast& Toast::animate(bool on) {
  Node::animate(on);
  return *this;
}

Toast& Toast::fade_sec(float s) {
  fade_sec_ = s;
  return *this;
}

Toast& Toast::font_size(float size) {
  font_size_ = size;
  return *this;
}

Toast& Toast::on_dismiss(DismissHandler handler) {
  on_dismiss_ = std::move(handler);
  return *this;
}

SizeF Toast::Measure(float max_w, float max_h) {
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  const float pad_x = 16.f;
  const float pad_y = 10.f;
  const float text_w =
      text_.empty() ? 0.f
                    : auralite::MeasureUiTextWidth(text_, fs, th.font_ui.c_str());
  const float hug_w = std::min(text_w + pad_x * 2.f, 360.f);
  const float hug_h = fs + pad_y * 2.f;
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void Toast::Paint(auralite::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  ColorF bg = th.surface;
  ColorF fg = th.text;
  ColorF stroke = th.border;
  if (variant_ == ToastVariant::Success) {
    bg = th.accent_soft;
    stroke = th.accent;
  } else if (variant_ == ToastVariant::Danger) {
    bg = th.danger;
    fg = th.text_on_accent;
    stroke = th.danger_hover;
  }
  const float r = 8.f;
  canvas.FillRoundedRect(bounds_, r, r, stroke);
  const float inset = 1.f;
  const RectF inner{bounds_.x + inset, bounds_.y + inset,
                    std::max(0.f, bounds_.w - inset * 2.f),
                    std::max(0.f, bounds_.h - inset * 2.f)};
  canvas.FillRoundedRect(inner, std::max(0.f, r - inset),
                         std::max(0.f, r - inset), bg);
  if (!text_.empty()) {
    canvas.DrawText(text_, inner, fg, ResolveFontSize(font_size_),
                    th.font_ui.c_str(), auralite::TextHAlign::Center);
  }
}

void Toast::OnMouseDown(const MouseEvent&) {
  if (on_dismiss_) {
    on_dismiss_();
  }
}

bool Toast::AccInvoke() {
  if (!on_dismiss_) {
    return false;
  }
  on_dismiss_();
  return true;
}

AccRole Toast::acc_role() const {
  if (acc_role_override_) {
    return *acc_role_override_;
  }
  return AccRole::Text;
}

std::wstring Toast::AccDefaultName() const {
  return text_;
}

}  // namespace auralite::ui
