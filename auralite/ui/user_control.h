#pragma once

#include "auralite/ui/node.h"

#include <functional>

namespace auralite {
class Canvas;
}

namespace auralite::ui {

// Layout placeholder that fills available space. Business drawing/input via
// callbacks or subclass overrides — no built-in chrome or data model.
class UserControl : public Node {
 public:
  using PaintHandler =
      std::function<void(auralite::Canvas& canvas, const RectF& bounds)>;
  using MouseHandler = std::function<void(const MouseEvent&)>;
  using KeyHandler = std::function<void(const KeyEvent&)>;

  UserControl();

  UserControl& on_paint(PaintHandler handler);
  UserControl& on_mouse_down(MouseHandler handler);
  UserControl& on_mouse_up(MouseHandler handler);
  UserControl& on_mouse_move(MouseHandler handler);
  UserControl& on_mouse_enter(MouseHandler handler);
  UserControl& on_mouse_leave(MouseHandler handler);
  UserControl& on_mouse_wheel(MouseHandler handler);
  UserControl& on_key(KeyHandler handler);
  UserControl& wants_mouse_wheel(bool want);

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseEnter(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;
  void OnMouseWheel(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

  bool WantsMouseWheel() const override;

 private:
  PaintHandler on_paint_;
  MouseHandler on_mouse_down_;
  MouseHandler on_mouse_up_;
  MouseHandler on_mouse_move_;
  MouseHandler on_mouse_enter_;
  MouseHandler on_mouse_leave_;
  MouseHandler on_mouse_wheel_;
  KeyHandler on_key_;
  bool wants_wheel_ = false;
};

}  // namespace auralite::ui
