#pragma once

#include "auralite/reactive/observe.h"
#include "auralite/ui/types.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace auralite::ui {

class Window;

class Node {
 public:
  virtual ~Node() = default;

  void AddChild(std::unique_ptr<Node> child);
  const std::vector<std::unique_ptr<Node>>& children() const { return children_; }

  RectF bounds() const { return bounds_; }
  void set_bounds(const RectF& r) { bounds_ = r; }

  Node* parent() const { return parent_; }
  // Event-routing parent only (not added to children_). Used by virtualized hosts.
  void set_event_parent(Node* parent) { parent_ = parent; }

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

  // Column/Row main-axis flex share (0 = not weighted; Fill still flexes as weight 1).
  Node& weight(float w);
  float weight() const { return weight_; }

  // Cross-axis align; if unset, Column/Row use their child_align default.
  // Named cross_align (not align) so Label can keep align(TextAlign) for text.
  Node& cross_align(Align a);
  Align cross_align() const { return cross_align_; }
  bool has_cross_align() const { return has_cross_align_; }

  // Absolute parent: child origin relative to parent content (ignored by Column/Row).
  Node& set_pos(float x, float y);
  float pos_x() const { return pos_x_; }
  float pos_y() const { return pos_y_; }
  bool has_pos() const { return has_pos_; }

  // Absolute edge anchors (distance to parent edges). Prefer over set_pos / own size
  // when both edges on an axis are set. Ignored by Column/Row/Tile/Tab.
  Node& left(float v);
  Node& top(float v);
  Node& right(float v);
  Node& bottom(float v);
  float left() const { return left_; }
  float top() const { return top_; }
  float right() const { return right_; }
  float bottom() const { return bottom_; }
  bool has_left() const { return has_left_; }
  bool has_top() const { return has_top_; }
  bool has_right() const { return has_right_; }
  bool has_bottom() const { return has_bottom_; }

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

  bool visible() const { return visible_; }
  Node& set_visible(bool v);

  // Keep subscription alive for this node's lifetime (unbind on destroy).
  void OwnSubscription(auralite::reactive::Subscription sub);

  Window* host_window() const { return host_window_; }
  void set_host_window(Window* w);

  // Optional id for FindByName (YAML `name`, bind templates).
  Node& set_name(std::string name);
  const std::string& name() const { return name_; }
  Node* FindByName(const std::string& name);
  const Node* FindByName(const std::string& name) const;

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
  bool visible_ = true;
  std::vector<auralite::reactive::Subscription> owned_subs_;
  Window* host_window_ = nullptr;
  std::string name_;
  ContextMenuHandler on_context_menu_;

  SizePolicy width_policy_ = SizePolicy::Hug;
  SizePolicy height_policy_ = SizePolicy::Hug;
  float preferred_w_ = 0.f;
  float preferred_h_ = 0.f;
  float weight_ = 0.f;
  Align cross_align_ = Align::Start;
  bool has_cross_align_ = false;
  float pos_x_ = 0.f;
  float pos_y_ = 0.f;
  bool has_pos_ = false;
  float left_ = 0.f;
  float top_ = 0.f;
  float right_ = 0.f;
  float bottom_ = 0.f;
  bool has_left_ = false;
  bool has_top_ = false;
  bool has_right_ = false;
  bool has_bottom_ = false;
};

}  // namespace auralite::ui
