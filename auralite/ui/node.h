#pragma once

#include "auralite/ui/types.h"

#include <functional>
#include <memory>
#include <string>
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

  SizePolicy width_policy() const { return width_policy_; }
  SizePolicy height_policy() const { return height_policy_; }
  float preferred_width() const { return preferred_w_; }
  float preferred_height() const { return preferred_h_; }

  Node& set_width_policy(SizePolicy p);
  Node& set_height_policy(SizePolicy p);
  Node& set_preferred_width(float w);
  Node& set_preferred_height(float h);

  // Convenience: Fixed + value, or Fill / Hug.
  Node& fixed_width(float w);
  Node& fixed_height(float h);
  Node& fill_width();
  Node& fill_height();
  Node& hug_width();
  Node& hug_height();

  // Default: claim the full available size (Fill/Fill).
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
  virtual void OnMouseEnter(const MouseEvent&) {}
  virtual void OnMouseLeave(const MouseEvent&) {}
  virtual void OnMouseWheel(const MouseEvent&) {}
  virtual void OnKey(const KeyEvent&) {}
  virtual void OnChar(wchar_t /*ch*/) {}
  virtual void OnFocus() {}
  virtual void OnBlur() {}

  // Right-click / WM_CONTEXTMENU. |screen_x/y| are screen coordinates for
  // TrackPopupMenu. Default invokes the optional handler (walks parents if
  // unset).
  using ContextMenuHandler = std::function<void(int screen_x, int screen_y)>;
  void set_on_context_menu(ContextMenuHandler handler);
  virtual void OnContextMenu(int screen_x, int screen_y);

  // Wheel: Window walks from hit node up to first ancestor that returns true.
  virtual bool WantsMouseWheel() const { return false; }

  // IME: composition is temporary underline text; result commits into the control.
  virtual bool WantsIme() const { return false; }
  virtual void OnImeComposition(const std::wstring& /*composition*/) {}
  virtual void OnImeResult(const std::wstring& /*result*/) {}
  virtual void OnImeEnd() {}

  // Drop GPU resources tied to the previous Canvas/device; default walks children.
  virtual void OnDeviceLost();

  bool focusable() const { return focusable_; }
  void set_focusable(bool v) { focusable_ = v; }
  bool focused() const { return focused_; }

 protected:
  friend class Window;
  void set_focused(bool v) { focused_ = v; }

  static bool ContainsPoint(const RectF& r, float x, float y);

  // Combine policy + preferred + intrinsic (hug) into a Measure result.
  SizeF ResolveSize(float max_w, float max_h, float hug_w, float hug_h) const;

  RectF bounds_{};
  std::vector<std::unique_ptr<Node>> children_;
  Node* parent_ = nullptr;
  bool focusable_ = false;
  bool focused_ = false;
  ContextMenuHandler on_context_menu_;

  SizePolicy width_policy_ = SizePolicy::Hug;
  SizePolicy height_policy_ = SizePolicy::Hug;
  float preferred_w_ = 0.f;
  float preferred_h_ = 0.f;
};

}  // namespace auralite::ui
