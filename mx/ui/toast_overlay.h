#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <windows.h>

namespace mx::ui {

class Toast;
class Window;

class ToastOverlay {
 public:
  ToastOverlay() = default;
  ~ToastOverlay();

  ToastOverlay(const ToastOverlay&) = delete;
  ToastOverlay& operator=(const ToastOverlay&) = delete;

  void Hide();
  void CancelFade();
  void FadeOut(float duration_sec, std::function<void()> done);
  bool Show(HWND owner, float owner_dpi, std::unique_ptr<Toast> toast);
  bool showing() const;
  Toast* toast() const;
  bool OwnsHwnd(HWND hwnd) const;

 private:
  bool Ensure(HWND owner);
  void Place(HWND owner, float owner_dpi, float dip_w, float dip_h);

  std::unique_ptr<Window> window_;
  uint64_t fade_id_ = 0;
};

}  // namespace mx::ui
