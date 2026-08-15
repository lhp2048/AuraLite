#include "auralite/ui/label.h"

#include <algorithm>

namespace auralite::ui {

Label& Label::text(const std::wstring& t) {
  text_ = t;
  return *this;
}

Label& Label::font_size(float size) {
  font_size_ = size;
  return *this;
}

Label& Label::color(const ColorF& c) {
  color_ = c;
  return *this;
}

Label& Label::align(TextAlign a) {
  align_ = a;
  return *this;
}

Label& Label::preferred_height(float h) {
  preferred_h_ = h;
  return *this;
}

auralite::TextHAlign Label::ToCanvasAlign(TextAlign a) {
  switch (a) {
    case TextAlign::Center:
      return auralite::TextHAlign::Center;
    case TextAlign::Right:
      return auralite::TextHAlign::Right;
    case TextAlign::Left:
    default:
      return auralite::TextHAlign::Left;
  }
}

SizeF Label::Measure(float max_w, float /*max_h*/) {
  const float h =
      preferred_h_ > 0.f ? preferred_h_ : (font_size_ + 8.f);
  // Prefer full available width so align left/center/right is visible.
  const float w = max_w > 0.f ? max_w : (font_size_ * 0.55f * text_.size() + 8.f);
  return SizeF{w, h};
}

void Label::Paint(auralite::Canvas& canvas) {
  if (text_.empty()) {
    return;
  }
  canvas.DrawText(text_, bounds_, color_, font_size_, L"Microsoft YaHei UI",
                  ToCanvasAlign(align_));
}

}  // namespace auralite::ui
