#pragma once

#include "auralite/ui/node.h"

namespace auralite::ui {

// Horizontal flex container: main axis = X, cross axis = Y.
class Row : public Node {
 public:
  Row();

  Row& padding(float all);
  Row& padding(float left, float top, float right, float bottom);
  Row& spacing(float s);
  Row& child_align(Align a);
  Row& main_align(Align a);

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
