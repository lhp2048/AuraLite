#include "mx/ui/user_control.h"

#include "mx/canvas.h"

namespace mx::ui {

UserControl::UserControl() {
  fill_width();
  fill_height();
}

UserControl& UserControl::on_paint(PaintHandler handler) {
  on_paint_ = std::move(handler);
  return *this;
}

UserControl& UserControl::on_mouse_down(MouseHandler handler) {
  on_mouse_down_ = std::move(handler);
  return *this;
}

UserControl& UserControl::on_mouse_up(MouseHandler handler) {
  on_mouse_up_ = std::move(handler);
  return *this;
}

UserControl& UserControl::on_mouse_move(MouseHandler handler) {
  on_mouse_move_ = std::move(handler);
  return *this;
}

UserControl& UserControl::on_mouse_enter(MouseHandler handler) {
  on_mouse_enter_ = std::move(handler);
  return *this;
}

UserControl& UserControl::on_mouse_leave(MouseHandler handler) {
  on_mouse_leave_ = std::move(handler);
  return *this;
}

UserControl& UserControl::on_mouse_wheel(MouseHandler handler) {
  on_mouse_wheel_ = std::move(handler);
  return *this;
}

UserControl& UserControl::on_key(KeyHandler handler) {
  on_key_ = std::move(handler);
  return *this;
}

UserControl& UserControl::wants_mouse_wheel(bool want) {
  wants_wheel_ = want;
  return *this;
}

SizeF UserControl::Measure(float max_w, float max_h) {
  return ResolveSize(max_w, max_h, max_w, max_h);
}

void UserControl::Paint(mx::Canvas& canvas) {
  if (on_paint_) {
    on_paint_(canvas, bounds_);
  }
  Node::Paint(canvas);
}

void UserControl::OnMouseDown(const MouseEvent& e) {
  if (on_mouse_down_) {
    on_mouse_down_(e);
  }
}

void UserControl::OnMouseUp(const MouseEvent& e) {
  if (on_mouse_up_) {
    on_mouse_up_(e);
  }
}

void UserControl::OnMouseMove(const MouseEvent& e) {
  if (on_mouse_move_) {
    on_mouse_move_(e);
  }
}

void UserControl::OnMouseEnter(const MouseEvent& e) {
  if (on_mouse_enter_) {
    on_mouse_enter_(e);
  }
}

void UserControl::OnMouseLeave(const MouseEvent& e) {
  if (on_mouse_leave_) {
    on_mouse_leave_(e);
  }
}

void UserControl::OnMouseWheel(const MouseEvent& e) {
  if (on_mouse_wheel_) {
    on_mouse_wheel_(e);
  }
}

void UserControl::OnKey(const KeyEvent& e) {
  if (on_key_) {
    on_key_(e);
  }
}

bool UserControl::WantsMouseWheel() const { return wants_wheel_; }

}  // namespace mx::ui
