#pragma once

#include "auralite/ui/factory.h"
#include "auralite/ui/node.h"
#include "auralite/ui/window.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace auralite::ui {

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
  void Push(const RectF& anchor_screen, std::unique_ptr<Node> root);
  void Dismiss();
  void DismissFrom(size_t level);
  bool is_open() const { return !stack_.empty(); }
  size_t depth() const { return stack_.size(); }

  // TLS: set for duration of Show/Push; Submenu uses Current().
  static PopupHost* Current();

  // Optional: wrap handler to Dismiss after invoke.
  std::function<void()> WrapDismiss(std::function<void()> fn);

 private:
  struct Layer {
    std::unique_ptr<Window> window;
  };
  std::vector<Layer> stack_;
  HWND owner_ = nullptr;
  bool owner_hooked_ = false;
  WNDPROC owner_old_proc_ = nullptr;

  void PlaceRoot(Window* w, POINT screen, SizeF content);
  void PlaceChild(Window* w, const RectF& anchor_screen, SizeF content);
  SizeF MeasureFit(Node* root);
  void ShowLayer(std::unique_ptr<Node> root, POINT screen_or_ignored,
                 const RectF* anchor_opt);
  bool HitAnyPopup(POINT screen) const;
  bool IsHwndInStack(HWND hwnd) const;
  void InstallOwnerHook();
  void UninstallOwnerHook();

  static LRESULT CALLBACK OwnerSubclassProc(HWND hwnd, UINT msg, WPARAM wparam,
                                            LPARAM lparam);
};

}  // namespace auralite::ui
