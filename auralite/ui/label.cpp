#include "auralite/ui/label.h"

#include "auralite/canvas.h"
#include "auralite/ui/theme.h"

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

float Label::font_size() const {
  return ResolveFontSize(font_size_);
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
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  const float hug_w =
      text_.empty()
          ? 0.f
          : (auralite::MeasureUiTextWidth(text_, fs, th.font_ui.c_str()) +
             4.f);
  const float hug_h = fs + 8.f;
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void Label::Paint(auralite::Canvas& canvas) {
  if (!visible() || text_.empty()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  const ColorF color = color_.value_or(th.text);
  canvas.DrawText(text_, bounds_, color, ResolveFontSize(font_size_),
                  th.font_ui.c_str(), ToCanvasAlign(align_));
}

}  // namespace auralite::ui
