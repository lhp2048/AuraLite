#pragma once

#include "auralite/canvas.h"
#include "auralite/ui/node.h"

#include <memory>

namespace auralite::ui {

class Window {
 public:
  Window();
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  bool Create(const wchar_t* title, int w, int h);
  void SetRoot(std::unique_ptr<Node> root);
  void Invalidate();
  HWND hwnd() const { return hwnd_; }

 private:
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                  LPARAM lparam);
  static Window* FromHwnd(HWND hwnd);
  static bool EnsureWindowClass(HINSTANCE instance);

  LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);
  void OnPaint();
  void OnSize(UINT width, UINT height);
  void NotifyDeviceLost();
  void ClearHover();
  void EnsureMouseLeaveTracking();
  void DispatchMouse(UINT msg, WPARAM wparam, LPARAM lparam);
  void DispatchKey(UINT msg, WPARAM wparam);

  static MouseButton ButtonFromMsg(UINT msg, WPARAM wparam);
  RectF ClientRectF() const;

  HWND hwnd_ = nullptr;
  auralite::Canvas canvas_;
  std::unique_ptr<Node> root_;
  bool layout_dirty_ = true;
  Node* mouse_capture_ = nullptr;
  Node* hovered_ = nullptr;
  bool tracking_mouse_leave_ = false;
};

}  // namespace auralite::ui
