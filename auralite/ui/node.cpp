#include "auralite/ui/node.h"

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

SizeF Node::Measure(float max_w, float max_h) {
  return SizeF{max_w, max_h};
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

void Node::OnDeviceLost() {
  for (auto& child : children_) {
    if (child) {
      child->OnDeviceLost();
    }
  }
}

}  // namespace auralite::ui
