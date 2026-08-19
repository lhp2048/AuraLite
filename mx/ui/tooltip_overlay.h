#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <windows.h>

namespace mx::ui {

class Window;

// Internal layered bubble owned by a host Window. Not a public window type:
// callers set Node::tooltip(); the host shows this after hover delay.
class TooltipOverlay {
 public:
  TooltipOverlay() = default;
  ~TooltipOverlay();

  TooltipOverlay(const TooltipOverlay&) = delete;
  TooltipOverlay& operator=(const TooltipOverlay&) = delete;

  void Hide();
  void Dismiss();
  void Show(HWND owner, float owner_dpi, const std::wstring& text, bool animate);
  bool OwnsHwnd(HWND hwnd) const;

 private:
  bool Ensure(HWND owner);
  void Place(float owner_dpi, float dip_w, float dip_h);
  void CancelFade();
  void FadeTo(float to_opacity, std::function<void()> done);

  std::unique_ptr<Window> window_;
  uint64_t fade_id_ = 0;
  bool animate_ = true;
};

}  // namespace mx::ui
