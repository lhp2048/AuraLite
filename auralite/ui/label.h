#pragma once

#include "auralite/ui/node.h"

#include <optional>
#include <string>

namespace auralite::ui {

class Label : public Node {
 public:
  Label();

  Label& text(const std::wstring& t);
  Label& font_size(float size);
  Label& color(const ColorF& c);
  Label& align(TextAlign a);
  // Fixed height; width stays Fill by default.
  Label& preferred_height(float h);

  const std::wstring& text() const { return text_; }
  float font_size() const;
  TextAlign align() const { return align_; }

  AccRole acc_role() const override;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  std::wstring AccDefaultName() const override;

 private:
  static auralite::TextHAlign ToCanvasAlign(TextAlign a);

  std::wstring text_;
  std::optional<float> font_size_;
  std::optional<ColorF> color_;
  TextAlign align_ = TextAlign::Left;
};

}  // namespace auralite::ui
