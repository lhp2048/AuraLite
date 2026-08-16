#pragma once

#include "auralite/canvas.h"
#include "auralite/ui/node.h"
#include "auralite/ui/theme.h"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace auralite::ui {

class Window {
 public:
  Window();
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  bool Create(const wchar_t* title, int w, int h);
  // Borderless owned popup (menus). Does not post WM_QUIT on destroy.
  bool CreatePopup(HWND owner, int width, int height);
  bool is_popup() const { return popup_mode_; }

  struct DialogOptions {
    bool topmost = true;
    bool center_on_owner = true;
  };
  // Borderless owned modal dialog. Never named CreateDialog (Win32 macro).
  bool CreateDialogWindow(HWND owner, int width, int height,
                          const DialogOptions& opt = {});
  bool CreateDialogWindow(HWND owner, const wchar_t* title, int width, int height,
                          const DialogOptions& opt = {});
  int RunModal();
  void EndModal(int result);
  int modal_result() const { return modal_result_; }
  bool is_dialog() const { return dialog_mode_; }

  // When true (default), WM_DESTROY posts WM_QUIT. Host apps that own the
  // message loop (e.g. Family Shell) should set false so closing a UI window
  // does not tear down the process.
  void set_quit_on_close(bool quit) { quit_on_close_ = quit; }
  bool quit_on_close() const { return quit_on_close_; }
  void SetRoot(std::unique_ptr<Node> root);
  // Detach root without destroying the window (Submenu return / PopupHost).
  std::unique_ptr<Node> ReleaseRoot();
  // Floating layer above root (Combo dropdown). |on_dismiss| runs on ClearPopup.
  // |anchor| click while open is left to the control (toggle), not dismissed here.
  void SetPopup(std::unique_ptr<Node> popup,
                std::function<void()> on_dismiss = {},
                Node* anchor = nullptr);
  void ClearPopup();
  // Safe while handling popup mouse events: runs ClearPopup after dispatch returns.
  void RequestClearPopup();
  Node* popup() const { return popup_.get(); }

  // PopupHost: invoked on WM_ACTIVATE(WA_INACTIVE) with the HWND gaining
  // activation. Host dismisses only when that HWND is outside the stack.
  void set_on_deactivate_outside(std::function<void(HWND)> cb) {
    on_deactivate_outside_ = std::move(cb);
  }

  void Invalidate();
  // Mark layout dirty and repaint (e.g. after visible toggles).
  void RequestLayout();
  // Shared alive flag for async/coroutines; cleared in destructor.
  std::shared_ptr<std::atomic_bool> alive_flag() const { return alive_; }
  HWND hwnd() const { return hwnd_; }
  float dpi() const { return dpi_; }

  // Ref-counted ~30fps Invalidate for indeterminate ProgressBar etc.
  void RegisterAnimation();
  void UnregisterAnimation();

  void SetFocusNode(Node* node);
  Node* focused_node() const { return focused_; }
  void FocusNext(bool reverse);

 private:
  static constexpr UINT_PTR kAnimTimerId = 1;
  static constexpr UINT_PTR kTooltipTimerId = 2;
  static constexpr UINT kTooltipDelayMs = 400;

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
  void ApplyDpiChange(UINT new_dpi, const RECT* suggested);

  void RestartTooltipTimer();
  void HideTooltip();
  void ShowTooltipFor(const Node* hit);
  bool EnsureTooltipWindow();
  void PlaceTooltipWindow(float dip_w, float dip_h);

  void PlaceDialogWindow(HWND owner, int width_dip, int height_dip,
                         const DialogOptions& opt);
  void ActivateDialogHwnd();
  void RestoreDialogOwner();

  static MouseButton ButtonFromMsg(UINT msg, WPARAM wparam);
  RectF ClientRectF() const;

  HWND hwnd_ = nullptr;
  float dpi_ = auralite::kDipDpi;
  auralite::Canvas canvas_;
  std::unique_ptr<Node> root_;
  std::unique_ptr<Node> popup_;
  std::function<void()> popup_dismiss_;
  Node* popup_anchor_ = nullptr;
  bool clear_popup_pending_ = false;
  bool layout_dirty_ = true;
  bool quit_on_close_ = true;
  bool popup_mode_ = false;
  bool dialog_mode_ = false;
  bool modal_running_ = false;
  HWND dialog_owner_ = nullptr;
  int modal_result_ = IDCANCEL;
  DialogOptions dialog_opt_{};
  std::function<void(HWND)> on_deactivate_outside_;
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

  std::unique_ptr<Window> tooltip_window_;
  std::wstring tooltip_text_;
  const std::wstring* tooltip_shown_text_ = nullptr;
};

}  // namespace auralite::ui
