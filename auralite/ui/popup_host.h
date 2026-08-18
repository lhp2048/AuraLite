#pragma once

#include "auralite/ui/factory.h"
#include "auralite/ui/node.h"
#include "auralite/ui/window.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace auralite::ui {

class Submenu;

// Where |screen| maps on the root popup when calling Show.
enum class PopupPlacement {
  kTopLeftAtPoint = 0,
  kBottomLeftAtPoint = 1,  // like TPM_BOTTOMALIGN (taskbar / tray)
};

struct PopupShowOptions {
  PopupPlacement placement = PopupPlacement::kTopLeftAtPoint;
  // false: clamp to monitor work area; true: full monitor (needed when the
  // anchor sits on a custom taskbar outside the work area).
  bool clamp_to_monitor = false;
  // Layered popup chrome. YAML menus: window.corner_radius / border_width.
  float corner_radius = 8.f;
  float border_width = 1.f;
  // Non-empty: Window::set_theme after create (overrides owner inherit).
  std::string theme;
};

// Owns a stack of layered menu windows. Explicit instance; TLS Current() is
// set while the stack is open for Submenu. Not created via Window::Create.
class PopupHost {
 public:
  PopupHost();
  ~PopupHost();  // Dismiss()

  PopupHost(const PopupHost&) = delete;
  PopupHost& operator=(const PopupHost&) = delete;

  void Show(HWND owner, POINT screen, std::unique_ptr<Node> root);
  void Show(HWND owner, POINT screen, std::unique_ptr<Node> root,
            PopupShowOptions options);
  // Swap the root layer's content without destroying the HWND (MenuBar hover
  // switch). Falls back to Show if the stack is empty.
  void Replace(std::unique_ptr<Node> root, POINT screen);
  void ShowFromYaml(HWND owner, POINT screen, const std::string& path_or_yaml,
                    const HandlerMap& handlers);
  // |return_to|: opener for sibling-dismiss; on DismissFrom of this layer,
  // ReleaseRoot() and give the node back via Submenu::content(...).
  // Returns leftover |root| on failure (caller must retain); nullptr on success.
  std::unique_ptr<Node> Push(const RectF& anchor_screen,
                             std::unique_ptr<Node> root,
                             Submenu* return_to = nullptr);
  void Dismiss();
  void DismissFrom(size_t level);
  // Mark dismiss for after the current popup DispatchMouse/Key returns.
  // Do not use MessageLoop PostTask: MessageBox nested pumps drain deferred
  // tasks before OnMouseUp returns (UAF). FlushPendingDismiss from Window.
  void RequestDismiss();
  void RequestDismissFrom(size_t level);
  // Runs pending dismiss; returns true if the stack was closed (caller must
  // not touch the popup Window afterward).
  bool FlushPendingDismiss();
  bool has_pending_dismiss() const { return dismiss_pending_; }
  bool is_open() const { return !stack_.empty(); }
  size_t depth() const { return stack_.size(); }
  // Index of |window| in the stack; nullopt if not found (never 0 for miss).
  std::optional<size_t> LevelOf(const Window* window) const;

  // Spec §4.3: while a child layer is open, hover/click on a non-opener
  // control in the parent popup dismisses from that child level.
  void OnPopupHit(Window* window, Node* hit);

  // TLS: set for duration of Show/Push; Submenu uses Current().
  static PopupHost* Current();

  // Optional: request dismiss after invoke; MessageBox-safe (flag + flush).
  std::function<void()> WrapDismiss(std::function<void()> fn);

 private:
  struct Layer {
    std::unique_ptr<Window> window;
    // Submenu that opened this layer (opened_from); also content return target.
    Submenu* return_to = nullptr;
  };
  std::vector<Layer> stack_;
  HWND owner_ = nullptr;
  bool owner_hooked_ = false;
  WNDPROC owner_old_proc_ = nullptr;
  bool mouse_hooked_ = false;
  bool dismiss_pending_ = false;
  size_t dismiss_pending_level_ = 0;
  // Run after FlushPendingDismiss closes the stack (e.g. About MessageBox).
  std::function<void()> after_dismiss_;
  PopupShowOptions show_options_;

  void PlaceRoot(Window* w, POINT screen, SizeF content, bool activate = true);
  void PlaceChild(Window* w, const RectF& anchor_screen, SizeF content);
  SizeF MeasureFit(Node* root);
  // On success ownership is in the new stack layer and returns nullptr.
  // On failure returns |root| so callers (Push/Submenu) can restore it.
  std::unique_ptr<Node> ShowLayer(std::unique_ptr<Node> root,
                                  POINT screen_or_ignored,
                                  const RectF* anchor_opt);
  bool HitAnyPopup(POINT screen) const;
  bool IsHwndInStack(HWND hwnd) const;
  void InstallOwnerHook();
  void UninstallOwnerHook();
  void InstallMouseHook();
  void UninstallMouseHook();
  void ClearOpenState();

  static LRESULT CALLBACK OwnerSubclassProc(HWND hwnd, UINT msg, WPARAM wparam,
                                            LPARAM lparam);
  static LRESULT CALLBACK MouseHookProc(int code, WPARAM wparam, LPARAM lparam);
};

}  // namespace auralite::ui
