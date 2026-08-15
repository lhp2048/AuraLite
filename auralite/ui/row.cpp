#include "auralite/ui/row.h"

#include <algorithm>

namespace auralite::ui {

Row& Row::padding(float all) {
  pad_l_ = pad_t_ = pad_r_ = pad_b_ = all;
  return *this;
}

Row& Row::padding(float left, float top, float right, float bottom) {
  pad_l_ = left;
  pad_t_ = top;
  pad_r_ = right;
  pad_b_ = bottom;
  return *this;
}

Row& Row::spacing(float s) {
  spacing_ = s;
  return *this;
}

SizeF Row::Measure(float max_w, float max_h) {
  const float inner_h = std::max(0.f, max_h - pad_t_ - pad_b_);
  float remaining_w = std::max(0.f, max_w - pad_l_ - pad_r_);
  float x = 0.f;
  float max_child_h = 0.f;

  for (size_t i = 0; i < children_.size(); ++i) {
    if (!children_[i]) {
      continue;
    }
    const SizeF s = children_[i]->Measure(remaining_w, inner_h);
    max_child_h = std::max(max_child_h, s.h);
    x += s.w;
    remaining_w = std::max(0.f, remaining_w - s.w);
    if (i + 1 < children_.size()) {
      x += spacing_;
      remaining_w = std::max(0.f, remaining_w - spacing_);
    }
  }

  const float hug_w = x + pad_l_ + pad_r_;
  const float hug_h = max_child_h + pad_t_ + pad_b_;
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void Row::Layout(const RectF& final_rect) {
  bounds_ = final_rect;

  const float inner_x = final_rect.x + pad_l_;
  const float inner_y = final_rect.y + pad_t_;
  const float inner_h = std::max(0.f, final_rect.h - pad_t_ - pad_b_);
  float remaining_w = std::max(0.f, final_rect.w - pad_l_ - pad_r_);
  float x = inner_x;

  for (size_t i = 0; i < children_.size(); ++i) {
    if (!children_[i]) {
      continue;
    }
    Node* child = children_[i].get();
    const SizeF s = child->Measure(remaining_w, inner_h);

    float child_w = s.w;
    float child_h = s.h;
    if (child->height_policy() == SizePolicy::Fill) {
      child_h = inner_h;
    } else {
      child_h = std::min(s.h, inner_h);
    }
    if (child->width_policy() == SizePolicy::Fill) {
      child_w = s.w;  // Measure already took remaining_w for Fill.
    } else {
      child_w = std::min(s.w, remaining_w);
    }

    child->Layout(RectF{x, inner_y, child_w, child_h});
    x += child_w;
    remaining_w = std::max(0.f, remaining_w - child_w);
    if (i + 1 < children_.size()) {
      x += spacing_;
      remaining_w = std::max(0.f, remaining_w - spacing_);
    }
  }
}

}  // namespace auralite::ui
