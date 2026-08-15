#pragma once

#include "auralite/ui/types.h"

#include <memory>
#include <vector>

namespace auralite::ui {

class Node {
 public:
  virtual ~Node() = default;

  void AddChild(std::unique_ptr<Node> child);
  const std::vector<std::unique_ptr<Node>>& children() const { return children_; }

  RectF bounds() const { return bounds_; }
  void set_bounds(const RectF& r) { bounds_ = r; }

  Node* parent() const { return parent_; }

  // Default: claim the full available size.
  virtual SizeF Measure(float max_w, float max_h);
  // Default: set bounds and give each child the same rect.
  virtual void Layout(const RectF& final_rect);
  // Default: paint children in order.
  virtual void Paint(auralite::Canvas& canvas);
  // Default: deepest child that contains (x,y), else this if inside bounds.
  virtual Node* HitTest(float x, float y);

  virtual void OnMouseDown(const MouseEvent&) {}
  virtual void OnMouseUp(const MouseEvent&) {}
  virtual void OnMouseMove(const MouseEvent&) {}
  virtual void OnMouseWheel(const MouseEvent&) {}
  virtual void OnKey(const KeyEvent&) {}
  virtual void OnFocus() {}
  virtual void OnBlur() {}

 protected:
  static bool ContainsPoint(const RectF& r, float x, float y);

  RectF bounds_{};
  std::vector<std::unique_ptr<Node>> children_;
  Node* parent_ = nullptr;
};

}  // namespace auralite::ui
