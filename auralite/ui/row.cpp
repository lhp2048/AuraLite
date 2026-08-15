#include "auralite/ui/row.h"

#include <algorithm>
#include <vector>

namespace auralite::ui {
namespace {

bool IsFlexWidth(const Node* c) {
  return c && c->width_policy() == SizePolicy::Fill;
}

float FlexWeightOf(const Node* c) {
  return c->weight() > 0.f ? c->weight() : 1.f;
}

Align ResolveCross(const Node* c, Align fallback) {
  return c->has_cross_align() ? c->cross_align() : fallback;
}

float CrossY(float inner_y, float inner_h, float child_h, Align a) {
  switch (a) {
    case Align::Center:
      return inner_y + (inner_h - child_h) * 0.5f;
    case Align::End:
      return inner_y + (inner_h - child_h);
    case Align::Start:
    default:
      return inner_y;
  }
}

float MainOffset(float free, Align a) {
  if (free <= 0.f) {
    return 0.f;
  }
  switch (a) {
    case Align::Center:
      return free * 0.5f;
    case Align::End:
      return free;
    case Align::Start:
    default:
      return 0.f;
  }
}

}  // namespace

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

Row& Row::child_align(Align a) {
  child_align_ = a;
  return *this;
}

Row& Row::main_align(Align a) {
  main_align_ = a;
  return *this;
}

SizeF Row::Measure(float max_w, float max_h) {
  const float inner_w = std::max(0.f, max_w - pad_l_ - pad_r_);
  const float inner_h = std::max(0.f, max_h - pad_t_ - pad_b_);

  float used_w = 0.f;
  float max_child_h = 0.f;
  int live = 0;
  for (const auto& c : children_) {
    if (!c) {
      continue;
    }
    ++live;
    if (IsFlexWidth(c.get())) {
      if (c->height_policy() == SizePolicy::Fixed && c->preferred_height() > 0.f) {
        max_child_h = std::max(max_child_h, c->preferred_height());
      } else {
        const SizeF s = c->Measure(0.f, inner_h);
        max_child_h = std::max(max_child_h, s.h);
      }
      continue;
    }
    const SizeF s = c->Measure(inner_w, inner_h);
    max_child_h = std::max(max_child_h, s.h);
    used_w += s.w;
  }
  if (live > 1) {
    used_w += spacing_ * static_cast<float>(live - 1);
  }

  return ResolveSize(max_w, max_h, used_w + pad_l_ + pad_r_,
                     max_child_h + pad_t_ + pad_b_);
}

void Row::Layout(const RectF& final_rect) {
  bounds_ = final_rect;

  const float inner_x = final_rect.x + pad_l_;
  const float inner_y = final_rect.y + pad_t_;
  const float inner_w = std::max(0.f, final_rect.w - pad_l_ - pad_r_);
  const float inner_h = std::max(0.f, final_rect.h - pad_t_ - pad_b_);

  std::vector<Node*> live;
  live.reserve(children_.size());
  for (auto& c : children_) {
    if (c) {
      live.push_back(c.get());
    }
  }
  if (live.empty()) {
    return;
  }

  const float gaps =
      spacing_ * static_cast<float>(std::max(0, static_cast<int>(live.size()) - 1));

  float fixed_w = 0.f;
  float total_weight = 0.f;
  bool any_flex = false;
  for (Node* c : live) {
    if (IsFlexWidth(c)) {
      any_flex = true;
      total_weight += FlexWeightOf(c);
    } else {
      fixed_w += c->Measure(inner_w, inner_h).w;
    }
  }

  float remaining = std::max(0.f, inner_w - fixed_w - gaps);
  float x = inner_x;
  if (!any_flex) {
    x += MainOffset(remaining, main_align_);
  }

  for (size_t i = 0; i < live.size(); ++i) {
    Node* child = live[i];
    float child_w = 0.f;
    SizeF s{};

    if (IsFlexWidth(child) && total_weight > 0.f) {
      child_w = remaining * (FlexWeightOf(child) / total_weight);
      s = child->Measure(child_w, inner_h);
    } else {
      s = child->Measure(inner_w, inner_h);
      child_w = s.w;
    }

    float child_h = s.h;
    if (child->height_policy() == SizePolicy::Fill) {
      child_h = inner_h;
    } else {
      child_h = std::min(s.h, inner_h);
    }

    const float y =
        CrossY(inner_y, inner_h, child_h, ResolveCross(child, child_align_));
    child->Layout(RectF{x, y, child_w, child_h});

    x += child_w;
    if (i + 1 < live.size()) {
      x += spacing_;
    }
  }
}

}  // namespace auralite::ui
