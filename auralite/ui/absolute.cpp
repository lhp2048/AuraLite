#include "auralite/ui/absolute.h"

#include <algorithm>

namespace auralite::ui {
namespace {

float AlignFree(float parent, float size, bool has_align, Align align,
                bool has_pos, float pos_fallback) {
  if (has_pos) {
    return pos_fallback;
  }
  if (!has_align) {
    return 0.f;
  }
  switch (align) {
    case Align::Center:
      return std::max(0.f, (parent - size) * 0.5f);
    case Align::End:
      return std::max(0.f, parent - size);
    case Align::Start:
    default:
      return 0.f;
  }
}

// Resolve child rect inside parent content size (parent origin at 0,0).
// Anchor edges win over set_pos; dual-edge sets size; else own Measure / Fill.
RectF ResolveAnchoredChild(Node* child, float parent_w, float parent_h) {
  const float fallback_x = child->has_pos() ? child->pos_x() : 0.f;
  const float fallback_y = child->has_pos() ? child->pos_y() : 0.f;

  float x = 0.f;
  float y = 0.f;
  float w = 0.f;
  float h = 0.f;

  // --- horizontal ---
  // Dual-edge: size from parent minus insets; drop width / Fill / Hug.
  if (child->has_left() && child->has_right()) {
    x = child->left();
    w = std::max(0.f, parent_w - child->left() - child->right());
  } else if (child->has_left()) {
    x = child->left();
    const float max_w = std::max(0.f, parent_w - child->left());
    const SizeF s = child->Measure(max_w, parent_h);
    if (child->width_policy() == SizePolicy::Fill) {
      w = max_w;
    } else {
      w = std::min(s.w, max_w);
    }
  } else if (child->has_right()) {
    const float max_w = std::max(0.f, parent_w - child->right());
    const SizeF s = child->Measure(max_w, parent_h);
    if (child->width_policy() == SizePolicy::Fill) {
      w = max_w;
      x = 0.f;
    } else {
      w = std::min(s.w, max_w);
      x = parent_w - child->right() - w;
    }
  } else {
    const SizeF s = child->Measure(parent_w, parent_h);
    if (child->width_policy() == SizePolicy::Fill) {
      w = parent_w;
      x = 0.f;
    } else {
      w = std::min(s.w, parent_w);
      x = AlignFree(parent_w, w, child->has_h_align(), child->h_align(),
                    child->has_pos(), fallback_x);
    }
  }

  // --- vertical ---
  // Dual-edge: size from parent minus insets; drop height / Fill / Hug.
  if (child->has_top() && child->has_bottom()) {
    y = child->top();
    h = std::max(0.f, parent_h - child->top() - child->bottom());
  } else if (child->has_top()) {
    y = child->top();
    const float max_h = std::max(0.f, parent_h - child->top());
    const SizeF s = child->Measure(w > 0.f ? w : parent_w, max_h);
    if (child->height_policy() == SizePolicy::Fill) {
      h = max_h;
    } else {
      h = std::min(s.h, max_h);
    }
  } else if (child->has_bottom()) {
    const float max_h = std::max(0.f, parent_h - child->bottom());
    const SizeF s = child->Measure(w > 0.f ? w : parent_w, max_h);
    if (child->height_policy() == SizePolicy::Fill) {
      h = max_h;
      y = 0.f;
    } else {
      h = std::min(s.h, max_h);
      y = parent_h - child->bottom() - h;
    }
  } else {
    const float max_h = parent_h;
    const SizeF s = child->Measure(w > 0.f ? w : parent_w, max_h);
    if (child->height_policy() == SizePolicy::Fill) {
      h = max_h;
      y = 0.f;
    } else {
      h = std::min(s.h, max_h);
      y = AlignFree(parent_h, h, child->has_v_align(), child->v_align(),
                    child->has_pos(), fallback_y);
    }
  }

  return RectF{x, y, w, h};
}

}  // namespace

Absolute::Absolute() {
  fill_width();
  fill_height();
}

SizeF Absolute::Measure(float max_w, float max_h) {
  float hug_w = 0.f;
  float hug_h = 0.f;
  for (const auto& c : children_) {
    if (!c) {
      continue;
    }
    const RectF r = ResolveAnchoredChild(c.get(), max_w, max_h);
    hug_w = std::max(hug_w, r.x + r.w);
    hug_h = std::max(hug_h, r.y + r.h);
    if (c->has_right()) {
      hug_w = std::max(hug_w, r.x + r.w + c->right());
    }
    if (c->has_bottom()) {
      hug_h = std::max(hug_h, r.y + r.h + c->bottom());
    }
  }
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void Absolute::Layout(const RectF& final_rect) {
  bounds_ = final_rect;
  for (auto& c : children_) {
    if (!c) {
      continue;
    }
    const RectF local =
        ResolveAnchoredChild(c.get(), final_rect.w, final_rect.h);
    c->Layout(RectF{final_rect.x + local.x, final_rect.y + local.y, local.w,
                    local.h});
  }
}

}  // namespace auralite::ui
