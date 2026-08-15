#pragma once

#include "auralite/canvas.h"
#include "auralite/ui/node.h"
#include "auralite/ui/theme.h"

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace auralite::ui {

class Window {
 public:
  Window();
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  bool Create(const wchar_t* title, int w, int h);
  void SetRoot(std::unique_ptr<Node> root);
  // Floating layer above root (Combo dropdown). |on_dismiss| runs on ClearPopup.
  // |anchor| click while open is left to the control (toggle), not dismissed here.
  void SetPopup(std::unique_ptr<Node> popup,
                std::function<void()> on_dismiss = {},
                Node* anchor = nullptr);
  void ClearPopup();
  // Safe while handling popup mouse events: runs ClearPopup after dispatch returns.
  void RequestClearPopup();
  Node* popup() const { return popup_.get(); }

  void Invalidate();
  // Mark layout dirty and repaint (e.g. after visible toggles).
  void RequestLayout();
  // Shared alive flag for async/coroutines; cleared in destructor.
  std::shared_ptr<std::atomic_bool> alive_flag() const { return alive_; }
  HWND hwnd() const { return hwnd_; }

  // Ref-counted ~30fps Invalidate for indeterminate ProgressBar etc.
  void RegisterAnimation();
  void UnregisterAnimation();

  void SetFocusNode(Node* node);
  Node* focused_node() const { return focused_; }
  void FocusNext(bool reverse);

 private:
  static constexpr UINT_PTR kAnimTimerId = 1;
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                  LPARAM lparam);
  static Window* FromHwnd(HWND hwnd);
  static bool EnsureWindowClass(HINSTANCE instance);
  static void CollectFocusable(Node* node, std::vector<Node*>* out);

  LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);
  void OnPaint();
  void OnSize(UINT width, UINT height);
  void NotifyDeviceLost();
  void ClearHover();
  void EnsureMouseLeaveTracking();
  void DispatchMouse(UINT msg, WPARAM wparam, LPARAM lparam);
  void DispatchContextMenu(WPARAM wparam, LPARAM lparam);
  void DispatchKey(UINT msg, WPARAM wparam);
  void DispatchChar(WPARAM wparam);
  void HandleImeComposition(LPARAM lparam);
  void DispatchImeChar(WPARAM wparam);
  void UpdateImeAssociation();
  void UpdateImeCandidatePos();
  void SyncPopupLayout();

  static MouseButton ButtonFromMsg(UINT msg, WPARAM wparam);
  RectF ClientRectF() const;

  HWND hwnd_ = nullptr;
  auralite::Canvas canvas_;
  std::unique_ptr<Node> root_;
  std::unique_ptr<Node> popup_;
  std::function<void()> popup_dismiss_;
  Node* popup_anchor_ = nullptr;
  bool clear_popup_pending_ = false;
  bool layout_dirty_ = true;
  Node* mouse_capture_ = nullptr;
  Node* hovered_ = nullptr;
  Node* focused_ = nullptr;
  bool tracking_mouse_leave_ = false;
  size_t ime_char_suppress_ = 0;
  int anim_clients_ = 0;
  bool invalidate_posted_ = false;
  Theme::InvalidateSink theme_sink_;
  std::shared_ptr<std::atomic_bool> alive_ =
      std::make_shared<std::atomic_bool>(true);
};

}  // namespace auralite::ui
