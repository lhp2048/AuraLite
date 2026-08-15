#pragma once

#include "auralite/ui/node.h"

namespace auralite::ui {

// Vertical flex container: main axis = Y, cross axis = X.
class Column : public Node {
 public:
  Column();

  Column& padding(float all);
  Column& padding(float left, float top, float right, float bottom);
  Column& spacing(float s);
  Column& child_align(Align a);
  // Pack leftover main-axis space when no Fill children (Start/Center/End).
  Column& main_align(Align a);

  float spacing() const { return spacing_; }
  Align child_align() const { return child_align_; }
  Align main_align() const { return main_align_; }

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;

 private:
  float pad_l_ = 0.f;
  float pad_t_ = 0.f;
  float pad_r_ = 0.f;
  float pad_b_ = 0.f;
  float spacing_ = 0.f;
  Align child_align_ = Align::Start;
  Align main_align_ = Align::Start;
};

}  // namespace auralite::ui
