#include "auralite/ui/column.h"

#include <algorithm>

namespace auralite::ui {

Column& Column::padding(float all) {
  pad_l_ = pad_t_ = pad_r_ = pad_b_ = all;
  return *this;
}

Column& Column::padding(float left, float top, float right, float bottom) {
  pad_l_ = left;
  pad_t_ = top;
  pad_r_ = right;
  pad_b_ = bottom;
  return *this;
}

Column& Column::spacing(float s) {
  spacing_ = s;
  return *this;
}

SizeF Column::Measure(float max_w, float max_h) {
  const float inner_w = std::max(0.f, max_w - pad_l_ - pad_r_);
  float remaining_h = std::max(0.f, max_h - pad_t_ - pad_b_);
  float y = 0.f;
  float max_child_w = 0.f;

  for (size_t i = 0; i < children_.size(); ++i) {
    if (!children_[i]) {
      continue;
    }
    const SizeF s = children_[i]->Measure(inner_w, remaining_h);
    max_child_w = std::max(max_child_w, s.w);
    y += s.h;
    remaining_h = std::max(0.f, remaining_h - s.h);
    if (i + 1 < children_.size()) {
      y += spacing_;
      remaining_h = std::max(0.f, remaining_h - spacing_);
    }
  }

  return SizeF{max_child_w + pad_l_ + pad_r_, y + pad_t_ + pad_b_};
}

void Column::Layout(const RectF& final_rect) {
  bounds_ = final_rect;

  const float inner_x = final_rect.x + pad_l_;
  const float inner_y = final_rect.y + pad_t_;
  const float inner_w = std::max(0.f, final_rect.w - pad_l_ - pad_r_);
  float remaining_h = std::max(0.f, final_rect.h - pad_t_ - pad_b_);
  float y = inner_y;

  for (size_t i = 0; i < children_.size(); ++i) {
    if (!children_[i]) {
      continue;
    }
    const SizeF s = children_[i]->Measure(inner_w, remaining_h);
    // Cross-axis stretch: child width fills column content width.
    children_[i]->Layout(RectF{inner_x, y, inner_w, s.h});
    y += s.h;
    remaining_h = std::max(0.f, remaining_h - s.h);
    if (i + 1 < children_.size()) {
      y += spacing_;
      remaining_h = std::max(0.f, remaining_h - spacing_);
    }
  }
}

}  // namespace auralite::ui
