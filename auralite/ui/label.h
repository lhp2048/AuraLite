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
  // Soft-wrap to width. When true, `trim` is ignored and height hugs line count.
  Label& wrap(bool enable);
  bool wrap() const { return wrap_; }
  // Single-line overflow. Default Clip (current paint). Ignored if wrap().
  Label& trim(TextTrim t);
  TextTrim trim() const { return trim_; }
  // Fixed height; width stays Fill by default.
  Label& preferred_height(float h);

  const std::wstring& text() const { return text_; }
  bool has_font_size() const { return font_size_.has_value(); }
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
  bool wrap_ = false;
  TextTrim trim_ = TextTrim::Clip;
};

}  // namespace auralite::ui
