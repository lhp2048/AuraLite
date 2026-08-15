#pragma once

#include "auralite/ui/node.h"

#include <string>

namespace auralite::ui {

class Label : public Node {
 public:
  Label& text(const std::wstring& t);
  Label& font_size(float size);
  Label& color(const ColorF& c);
  Label& align(TextAlign a);
  Label& preferred_height(float h);

  const std::wstring& text() const { return text_; }
  float font_size() const { return font_size_; }
  TextAlign align() const { return align_; }

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

 private:
  static auralite::TextHAlign ToCanvasAlign(TextAlign a);

  std::wstring text_;
  float font_size_ = 16.f;
  ColorF color_ = ColorF::FromRgb(30, 40, 55);
  TextAlign align_ = TextAlign::Left;
  float preferred_h_ = 0.f;  // 0 = font_size + padding
};

}  // namespace auralite::ui
