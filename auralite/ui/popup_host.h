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

// Owns a stack of WS_POPUP menu windows (CreatePopup). Explicit instance;
// TLS Current() is set while the stack is open for Submenu.
class PopupHost {
 public:
  PopupHost();
  ~PopupHost();  // Dismiss()

  PopupHost(const PopupHost&) = delete;
  PopupHost& operator=(const PopupHost&) = delete;

  void Show(HWND owner, POINT screen, std::unique_ptr<Node> root);
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
  // Defer Dismiss* until after the current Win32/UI handler returns (non-nestable
  // PostTask). Required when closing from Button::on_click / Esc / hooks —
  // synchronous DestroyWindow UAF's the popup still on the call stack.
  void RequestDismiss();
  void RequestDismissFrom(size_t level);
  bool is_open() const { return !stack_.empty(); }
  size_t depth() const { return stack_.size(); }
  // Index of |window| in the stack; nullopt if not found (never 0 for miss).
  std::optional<size_t> LevelOf(const Window* window) const;

  // Spec §4.3: while a child layer is open, hover/click on a non-opener
  // control in the parent popup dismisses from that child level.
  void OnPopupHit(Window* window, Node* hit);

  // TLS: set for duration of Show/Push; Submenu uses Current().
  static PopupHost* Current();

  // Optional: wrap handler to RequestDismiss after invoke (safe from on_click).
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
  bool dismiss_posted_ = false;
  size_t dismiss_posted_level_ = 0;
  // Shared with deferred dismiss tasks so ~PopupHost cannot UAF.
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);

  void PlaceRoot(Window* w, POINT screen, SizeF content);
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
