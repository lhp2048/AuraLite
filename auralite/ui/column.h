#pragma once

#include "auralite/ui/node.h"

namespace auralite::ui {

// Vertical flex container: main axis = Y, cross axis = X (stretch).
class Column : public Node {
 public:
  Column& padding(float all);
  Column& padding(float left, float top, float right, float bottom);
  Column& spacing(float s);

  float spacing() const { return spacing_; }

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;

 private:
  float pad_l_ = 0.f;
  float pad_t_ = 0.f;
  float pad_r_ = 0.f;
  float pad_b_ = 0.f;
  float spacing_ = 0.f;
};

}  // namespace auralite::ui
