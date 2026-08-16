#include "auralite/ui/toast_overlay.h"

#include "auralite/canvas.h"
#include "auralite/ui/toast.h"
#include "auralite/ui/window.h"

#include <algorithm>
#include <cmath>

namespace auralite::ui {

ToastOverlay::~ToastOverlay() { Hide(); }

void ToastOverlay::Hide() {
  CancelFade();
  if (window_ && window_->hwnd()) {
    ShowWindow(window_->hwnd(), SW_HIDE);
    window_->SetRoot(std::unique_ptr<Node>{});
    window_->set_layered_opacity(1.f);
  }
}

void ToastOverlay::CancelFade() {
  if (fade_id_ && window_) {
    window_->CancelAnimation(fade_id_);
    fade_id_ = 0;
  }
}

void ToastOverlay::FadeOut(float duration_sec, std::function<void()> done) {
  if (!window_ || !window_->hwnd() || duration_sec <= 0.f) {
    if (done) {
      done();
    }
    return;
  }
  CancelFade();
  fade_id_ = window_->Animate(
      duration_sec, Easing::EaseOutCubic,
      [this](float t) { window_->set_layered_opacity(1.f - t); },
      [this, done = std::move(done)]() {
        fade_id_ = 0;
        if (done) {
          done();
        }
      });
}

bool ToastOverlay::showing() const {
  return window_ && window_->hwnd() && IsWindowVisible(window_->hwnd());
}

Toast* ToastOverlay::toast() const {
  return window_ ? dynamic_cast<Toast*>(window_->root()) : nullptr;
}

bool ToastOverlay::OwnsHwnd(HWND hwnd) const {
  return window_ && window_->hwnd() && hwnd == window_->hwnd();
}

bool ToastOverlay::Ensure(HWND owner) {
  if (window_ && window_->hwnd()) {
    return true;
  }
  window_ = std::make_unique<Window>();
  const DWORD ex =
      WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED;
  if (!window_->CreateLayeredTool(owner, 1, 1, ex)) {
    window_.reset();
    return false;
  }
  return true;
}

void ToastOverlay::Place(HWND owner, float owner_dpi, float dip_w,
                         float dip_h) {
  if (!window_ || !window_->hwnd() || !owner) {
    return;
  }
  const int pw =
      static_cast<int>(std::ceil(auralite::PxFromDip(dip_w, owner_dpi)));
  const int ph =
      static_cast<int>(std::ceil(auralite::PxFromDip(dip_h, owner_dpi)));
  const int margin =
      static_cast<int>(std::ceil(auralite::PxFromDip(16.f, owner_dpi)));

  RECT cr{};
  GetClientRect(owner, &cr);
  POINT bl{cr.left, cr.bottom};
  POINT br{cr.right, cr.bottom};
  ClientToScreen(owner, &bl);
  ClientToScreen(owner, &br);
  int x = bl.x + (br.x - bl.x - pw) / 2;
  int y = bl.y - ph - margin;

  HMONITOR mon = MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi = {};
  mi.cbSize = sizeof(mi);
  if (mon && GetMonitorInfoW(mon, &mi)) {
    const RECT& work = mi.rcWork;
    if (x + pw > work.right) {
      x = work.right - pw;
    }
    if (y + ph > work.bottom) {
      y = work.bottom - ph;
    }
    if (x < work.left) {
      x = work.left;
    }
    if (y < work.top) {
      y = work.top;
    }
  }

  SetWindowPos(window_->hwnd(), HWND_TOPMOST, x, y, pw, ph, SWP_NOACTIVATE);
}

bool ToastOverlay::Show(HWND owner, float owner_dpi,
                        std::unique_ptr<Toast> toast) {
  if (!toast || !owner) {
    return false;
  }
  if (!Ensure(owner)) {
    return false;
  }
  CancelFade();
  const SizeF sz = toast->Measure(360.f, 120.f);
  const float tw = std::max(sz.w, 80.f);
  const float th = std::max(sz.h, 32.f);
  window_->set_layered_opacity(1.f);
  window_->SetRoot(std::move(toast));
  Place(owner, owner_dpi, tw, th);
  ShowWindow(window_->hwnd(), SW_SHOWNOACTIVATE);
  window_->OnPaint();
  return true;
}

}  // namespace auralite::ui
