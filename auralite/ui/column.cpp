#include "auralite/ui/column.h"

#include <algorithm>
#include <vector>

namespace auralite::ui {
namespace {

// Only main-axis Fill participates in weight share; Fixed/Hug ignore weight.
bool IsFlexHeight(const Node* c) {
  return c && c->height_policy() == SizePolicy::Fill;
}

float FlexWeightOf(const Node* c) {
  return c->weight() > 0.f ? c->weight() : 1.f;
}

Align ResolveH(const Node* c, Align fallback) {
  return c->has_h_align() ? c->h_align() : fallback;
}

float CrossX(float inner_x, float inner_w, float child_w, Align a) {
  switch (a) {
    case Align::Center:
      return inner_x + (inner_w - child_w) * 0.5f;
    case Align::End:
      return inner_x + (inner_w - child_w);
    case Align::Start:
    default:
      return inner_x;
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

Column::Column() {
  fill_width();
  clip_children(true);
}

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

Column& Column::h_align(Align a) {
  child_h_align_ = a;
  Node::h_align(a);
  return *this;
}

Column& Column::v_align(Align a) {
  pack_v_align_ = a;
  Node::v_align(a);
  return *this;
}

SizeF Column::Measure(float max_w, float max_h) {
  const float inner_w = std::max(0.f, max_w - pad_l_ - pad_r_);
  const float inner_h = std::max(0.f, max_h - pad_t_ - pad_b_);

  float used_h = 0.f;
  float max_child_w = 0.f;
  int live = 0;
  for (const auto& c : children_) {
    if (!c || !c->visible()) {
      continue;
    }
    ++live;
    if (IsFlexHeight(c.get())) {
      if (c->width_policy() == SizePolicy::Fixed && c->preferred_width() > 0.f) {
        max_child_w = std::max(max_child_w, c->preferred_width());
      } else {
        const SizeF s = c->Measure(inner_w, 0.f);
        max_child_w = std::max(max_child_w, s.w);
      }
      continue;
    }
    const SizeF s = c->Measure(inner_w, inner_h);
    max_child_w = std::max(max_child_w, s.w);
    used_h += s.h;
  }
  if (live > 1) {
    used_h += spacing_ * static_cast<float>(live - 1);
  }

  return ResolveSize(max_w, max_h, max_child_w + pad_l_ + pad_r_,
                     used_h + pad_t_ + pad_b_);
}

void Column::Layout(const RectF& final_rect) {
  bounds_ = final_rect;

  const float inner_x = final_rect.x + pad_l_;
  const float inner_y = final_rect.y + pad_t_;
  const float inner_w = std::max(0.f, final_rect.w - pad_l_ - pad_r_);
  const float inner_h = std::max(0.f, final_rect.h - pad_t_ - pad_b_);

  std::vector<Node*> live;
  live.reserve(children_.size());
  for (auto& c : children_) {
    if (c && c->visible()) {
      live.push_back(c.get());
    }
  }
  if (live.empty()) {
    return;
  }

  const float gaps =
      spacing_ * static_cast<float>(std::max(0, static_cast<int>(live.size()) - 1));

  float fixed_h = 0.f;
  float total_weight = 0.f;
  bool any_flex = false;
  for (Node* c : live) {
    if (IsFlexHeight(c)) {
      any_flex = true;
      total_weight += FlexWeightOf(c);
    } else {
      fixed_h += c->Measure(inner_w, inner_h).h;
    }
  }

  float remaining = std::max(0.f, inner_h - fixed_h - gaps);
  float y = inner_y;
  if (!any_flex) {
    y += MainOffset(remaining, pack_v_align_);
  }

  for (size_t i = 0; i < live.size(); ++i) {
    Node* child = live[i];
    float child_h = 0.f;
    SizeF s{};

    if (IsFlexHeight(child) && total_weight > 0.f) {
      child_h = remaining * (FlexWeightOf(child) / total_weight);
      s = child->Measure(inner_w, child_h);
    } else {
      s = child->Measure(inner_w, inner_h);
      child_h = s.h;
    }

    float child_w = s.w;
    if (child->width_policy() == SizePolicy::Fill) {
      child_w = inner_w;
    } else {
      child_w = std::min(s.w, inner_w);
    }

    const float x =
        CrossX(inner_x, inner_w, child_w, ResolveH(child, child_h_align_));
    child->Layout(RectF{x, y, child_w, child_h});

    y += child_h;
    if (i + 1 < live.size()) {
      y += spacing_;
    }
  }
}

}  // namespace auralite::ui
