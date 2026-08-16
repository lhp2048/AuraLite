#pragma once

#include "auralite/ui/node.h"

namespace auralite::ui {

// Vertical stack. h_align = children left/right default; v_align = pack when no Fill.
class Column : public Node {
 public:
  Column();

  Column& padding(float all);
  Column& padding(float left, float top, float right, float bottom);
  Column& spacing(float s);
  Column& h_align(Align a) override;
  Column& v_align(Align a) override;

  float spacing() const { return spacing_; }

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;

 private:
  float pad_l_ = 0.f;
  float pad_t_ = 0.f;
  float pad_r_ = 0.f;
  float pad_b_ = 0.f;
  float spacing_ = 0.f;
  Align child_h_align_ = Align::Start;
  Align pack_v_align_ = Align::Start;
};

}  // namespace auralite::ui
