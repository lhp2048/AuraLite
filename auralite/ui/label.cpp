#include "auralite/ui/label.h"

#include "auralite/canvas.h"

namespace auralite::ui {

Label::Label() {
  fill_width();
  hug_height();
}

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
  fixed_height(h);
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

SizeF Label::Measure(float max_w, float max_h) {
  const float hug_w =
      text_.empty() ? 0.f
                    : (auralite::MeasureUiTextWidth(text_, font_size_) + 4.f);
  const float hug_h = font_size_ + 8.f;
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void Label::Paint(auralite::Canvas& canvas) {
  if (text_.empty()) {
    return;
  }
  canvas.DrawText(text_, bounds_, color_, font_size_, L"Microsoft YaHei UI",
                  ToCanvasAlign(align_));
}

}  // namespace auralite::ui
