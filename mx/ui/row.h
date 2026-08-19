#pragma once

#include "mx/ui/node.h"

namespace mx::ui {

// Horizontal stack. v_align = children top/bottom default; h_align = pack when no Fill.
class Row : public Node {
 public:
  Row();

  Row& padding(float all);
  Row& padding(float left, float top, float right, float bottom);
  Row& spacing(float s);
  Row& h_align(Align a) override;
  Row& v_align(Align a) override;

  float spacing() const { return spacing_; }

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;

 private:
  float pad_l_ = 0.f;
  float pad_t_ = 0.f;
  float pad_r_ = 0.f;
  float pad_b_ = 0.f;
  float spacing_ = 0.f;
  Align pack_h_align_ = Align::Start;
  Align child_v_align_ = Align::Start;
};

}  // namespace mx::ui
