#include "auralite/ui/node.h"

#include <algorithm>

namespace auralite::ui {

bool Node::ContainsPoint(const RectF& r, float x, float y) {
  return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

void Node::AddChild(std::unique_ptr<Node> child) {
  if (!child) {
    return;
  }
  child->parent_ = this;
  children_.push_back(std::move(child));
}

Node& Node::set_width_policy(SizePolicy p) {
  width_policy_ = p;
  return *this;
}

Node& Node::set_height_policy(SizePolicy p) {
  height_policy_ = p;
  return *this;
}

Node& Node::set_preferred_width(float w) {
  preferred_w_ = w;
  return *this;
}

Node& Node::set_preferred_height(float h) {
  preferred_h_ = h;
  return *this;
}

Node& Node::fixed_width(float w) {
  width_policy_ = SizePolicy::Fixed;
  preferred_w_ = w;
  return *this;
}

Node& Node::fixed_height(float h) {
  height_policy_ = SizePolicy::Fixed;
  preferred_h_ = h;
  return *this;
}

Node& Node::fill_width() {
  width_policy_ = SizePolicy::Fill;
  return *this;
}

Node& Node::fill_height() {
  height_policy_ = SizePolicy::Fill;
  return *this;
}

Node& Node::hug_width() {
  width_policy_ = SizePolicy::Hug;
  return *this;
}

Node& Node::hug_height() {
  height_policy_ = SizePolicy::Hug;
  return *this;
}

Node& Node::weight(float w) {
  weight_ = std::max(0.f, w);
  return *this;
}

Node& Node::cross_align(Align a) {
  cross_align_ = a;
  has_cross_align_ = true;
  return *this;
}

Node& Node::set_pos(float x, float y) {
  pos_x_ = x;
  pos_y_ = y;
  has_pos_ = true;
  return *this;
}

Node& Node::left(float v) {
  left_ = v;
  has_left_ = true;
  return *this;
}

Node& Node::top(float v) {
  top_ = v;
  has_top_ = true;
  return *this;
}

Node& Node::right(float v) {
  right_ = v;
  has_right_ = true;
  return *this;
}

Node& Node::bottom(float v) {
  bottom_ = v;
  has_bottom_ = true;
  return *this;
}

SizeF Node::ResolveSize(float max_w, float max_h, float hug_w,
                        float hug_h) const {
  float w = hug_w;
  float h = hug_h;

  switch (width_policy_) {
    case SizePolicy::Fixed:
      w = preferred_w_ > 0.f ? preferred_w_ : hug_w;
      break;
    case SizePolicy::Hug:
      w = hug_w;
      break;
    case SizePolicy::Fill:
      w = max_w > 0.f ? max_w : (preferred_w_ > 0.f ? preferred_w_ : hug_w);
      break;
  }

  switch (height_policy_) {
    case SizePolicy::Fixed:
      h = preferred_h_ > 0.f ? preferred_h_ : hug_h;
      break;
    case SizePolicy::Hug:
      h = hug_h;
      break;
    case SizePolicy::Fill:
      h = max_h > 0.f ? max_h : (preferred_h_ > 0.f ? preferred_h_ : hug_h);
      break;
  }

  if (max_w > 0.f) {
    w = std::min(w, max_w);
  }
  if (max_h > 0.f) {
    h = std::min(h, max_h);
  }
  return SizeF{std::max(0.f, w), std::max(0.f, h)};
}

SizeF Node::Measure(float max_w, float max_h) {
  return ResolveSize(max_w, max_h, max_w, max_h);
}

void Node::Layout(const RectF& final_rect) {
  bounds_ = final_rect;
  for (auto& child : children_) {
    if (child) {
      child->Layout(final_rect);
    }
  }
}

void Node::Paint(auralite::Canvas& canvas) {
  for (auto& child : children_) {
    if (child) {
      child->Paint(canvas);
    }
  }
}

Node* Node::HitTest(float x, float y) {
  if (!ContainsPoint(bounds_, x, y)) {
    return nullptr;
  }
  // Front-most (last painted) child first.
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    if (!*it) {
      continue;
    }
    if (Node* hit = (*it)->HitTest(x, y)) {
      return hit;
    }
  }
  return this;
}

Node& Node::set_name(std::string name) {
  name_ = std::move(name);
  return *this;
}

Node* Node::FindByName(const std::string& name) {
  if (name.empty()) {
    return nullptr;
  }
  if (name_ == name) {
    return this;
  }
  for (auto& child : children_) {
    if (!child) {
      continue;
    }
    if (Node* hit = child->FindByName(name)) {
      return hit;
    }
  }
  return nullptr;
}

const Node* Node::FindByName(const std::string& name) const {
  return const_cast<Node*>(this)->FindByName(name);
}

void Node::set_on_context_menu(ContextMenuHandler handler) {
  on_context_menu_ = std::move(handler);
}

void Node::OnContextMenu(int screen_x, int screen_y) {
  if (on_context_menu_) {
    on_context_menu_(screen_x, screen_y);
    return;
  }
  if (parent_) {
    parent_->OnContextMenu(screen_x, screen_y);
  }
}

void Node::OnDeviceLost() {
  for (auto& child : children_) {
    if (child) {
      child->OnDeviceLost();
    }
  }
}

}  // namespace auralite::ui
