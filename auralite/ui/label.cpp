#include "auralite/ui/label.h"

#include "auralite/canvas.h"
#include "auralite/ui/text_layout.h"
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

Label& Label::wrap(bool enable) {
  wrap_ = enable;
  return *this;
}

Label& Label::trim(TextTrim t) {
  trim_ = t;
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
  const float line_h = fs + 8.f;
  const wchar_t* font = th.font_ui.c_str();

  if (wrap_) {
    std::vector<std::wstring> lines;
    WrapUiText(text_, max_w, fs, font, &lines);
    float hug_w = 0.f;
    for (const auto& line : lines) {
      if (line.empty()) {
        continue;
      }
      const float w = auralite::MeasureUiTextWidth(line, fs, font) + 4.f;
      if (w > hug_w) {
        hug_w = w;
      }
    }
    const float hug_h = line_h * static_cast<float>(
                                     lines.empty() ? 1 : lines.size());
    return ResolveSize(max_w, max_h, hug_w, hug_h);
  }

  const float hug_w =
      text_.empty()
          ? 0.f
          : (auralite::MeasureUiTextWidth(text_, fs, font) + 4.f);
  return ResolveSize(max_w, max_h, hug_w, line_h);
}

void Label::Paint(auralite::Canvas& canvas) {
  if (!visible() || text_.empty()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  const ColorF color = color_.value_or(th.text);
  const float fs = ResolveFontSize(font_size_);
  const wchar_t* font = th.font_ui.c_str();
  const auralite::TextHAlign ha = ToCanvasAlign(align_);

  if (wrap_) {
    std::vector<std::wstring> lines;
    WrapUiText(text_, bounds_.w, fs, font, &lines);
    const float line_h = fs + 8.f;
    float y = bounds_.y;
    for (const auto& line : lines) {
      if (!line.empty()) {
        canvas.DrawText(line, RectF{bounds_.x, y, bounds_.w, line_h}, color, fs,
                        font, ha);
      }
      y += line_h;
    }
    return;
  }

  std::wstring shown = text_;
  if (trim_ != TextTrim::Clip) {
    shown = EllipsizeUiText(text_, bounds_.w, fs, font, trim_);
  }
  if (shown.empty()) {
    return;
  }
  canvas.DrawText(shown, bounds_, color, fs, font, ha);
}

AccRole Label::acc_role() const {
  return AccRole::Text;
}

std::wstring Label::AccDefaultName() const {
  return text_;
}

}  // namespace auralite::ui
