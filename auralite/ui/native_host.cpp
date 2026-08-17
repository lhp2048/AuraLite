#include "auralite/ui/native_host.h"

#include "auralite/canvas.h"
#include "auralite/ui/window.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace auralite::ui {
namespace {

std::vector<NativeHost*> g_live;

}  // namespace

NativeHost::NativeHost() {
  fill_width();
  fill_height();
}

NativeHost::~NativeHost() {
  Detach();
}

void NativeHost::OrphanTree(Node* root) {
  if (!root) {
    return;
  }
  if (auto* host = dynamic_cast<NativeHost*>(root)) {
    host->Detach();
  }
  for (const auto& child : root->children()) {
    OrphanTree(child.get());
  }
}

void NativeHost::RegisterLive(NativeHost* host) {
  if (!host) {
    return;
  }
  for (NativeHost* h : g_live) {
    if (h == host) {
      return;
    }
  }
  g_live.push_back(host);
}

void NativeHost::UnregisterLive(NativeHost* host) {
  g_live.erase(std::remove(g_live.begin(), g_live.end(), host), g_live.end());
}

void NativeHost::RedrawGuests(HWND parent, const RECT& present_px) {
  if (!parent) {
    return;
  }
  for (NativeHost* host : g_live) {
    if (!host || !host->hwnd_ || !IsWindow(host->hwnd_)) {
      continue;
    }
    if (GetParent(host->hwnd_) != parent || !IsWindowVisible(host->hwnd_)) {
      continue;
    }
    RECT guest{host->synced_x_, host->synced_y_,
               host->synced_x_ + host->synced_w_,
               host->synced_y_ + host->synced_h_};
    RECT hit = {};
    if (!IntersectRect(&hit, &guest, &present_px)) {
      continue;
    }
    RedrawWindow(host->hwnd_, nullptr, nullptr,
                 RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE |
                     RDW_ALLCHILDREN);
  }
}

NativeHost& NativeHost::Attach(HWND hwnd) {
  return Attach(hwnd, NativeLifetime::Owned);
}

NativeHost& NativeHost::AttachBorrowed(HWND hwnd) {
  return Attach(hwnd, NativeLifetime::Borrowed);
}

NativeHost& NativeHost::Attach(HWND hwnd, NativeLifetime life) {
  if (hwnd_ == hwnd) {
    owned_ = (life == NativeLifetime::Owned);
    SyncNative();
    return *this;
  }
  Detach();
  if (!hwnd || !IsWindow(hwnd)) {
    return *this;
  }
  hwnd_ = hwnd;
  orig_parent_ = GetParent(hwnd_);
  orig_style_ = GetWindowLongW(hwnd_, GWL_STYLE);
  attached_ = true;
  owned_ = (life == NativeLifetime::Owned);
  RegisterLive(this);
  AdoptParent();
  SyncNative();
  return *this;
}

void NativeHost::Detach() {
  Drop(true);
}

HWND NativeHost::Release() {
  HWND guest = hwnd_;
  Drop(false);
  if (guest && IsWindow(guest)) {
    return guest;
  }
  return nullptr;
}

void NativeHost::Drop(bool destroy_owned) {
  if (!attached_) {
    hwnd_ = nullptr;
    owned_ = true;
    return;
  }
  HWND guest = hwnd_;
  HWND orig_parent = orig_parent_;
  LONG orig_style = orig_style_;
  const bool owned = owned_;
  attached_ = false;
  hwnd_ = nullptr;
  orig_parent_ = nullptr;
  orig_style_ = 0;
  owned_ = true;
  synced_x_ = 0;
  synced_y_ = 0;
  synced_w_ = 0;
  synced_h_ = 0;
  synced_visible_ = false;
  UnregisterLive(this);
  if (!guest || !IsWindow(guest)) {
    return;
  }
  if (destroy_owned && owned) {
    DestroyWindow(guest);
    return;
  }
  ShowWindow(guest, SW_HIDE);
  SetParent(guest, orig_parent);
  SetWindowLongW(guest, GWL_STYLE, orig_style);
  SetWindowPos(guest, nullptr, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                   SWP_FRAMECHANGED);
}

SizeF NativeHost::Measure(float max_w, float max_h) {
  return ResolveSize(max_w, max_h, max_w, max_h);
}

void NativeHost::Layout(const RectF& final_rect) {
  Node::Layout(final_rect);
  SyncNative();
}

void NativeHost::OnHostWindowChanged() {
  if (attached_) {
    AdoptParent();
    SyncNative();
  }
}

bool NativeHost::ShownInTree() const {
  for (const Node* n = this; n; n = n->parent()) {
    if (!n->visible()) {
      return false;
    }
  }
  return true;
}

void NativeHost::AdoptParent() {
  if (!attached_ || !hwnd_ || !IsWindow(hwnd_)) {
    return;
  }
  Window* w = host_window();
  HWND parent = (w && w->hwnd()) ? w->hwnd() : nullptr;
  if (!parent) {
    HideGuest();
    return;
  }
  if (GetParent(hwnd_) != parent) {
    SetParent(hwnd_, parent);
  }
  const LONG style = GetWindowLongW(hwnd_, GWL_STYLE);
  LONG next = style | WS_CHILD | WS_CLIPSIBLINGS;
  next &= ~WS_POPUP;
  if (next == style) {
    return;
  }
  SetWindowLongW(hwnd_, GWL_STYLE, next);
  SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                   SWP_NOCOPYBITS | SWP_FRAMECHANGED);
}

void NativeHost::InvalidateSyncedRect() {
  Window* w = host_window();
  if (!w || !w->hwnd() || synced_w_ <= 0 || synced_h_ <= 0) {
    return;
  }
  RECT rc{synced_x_, synced_y_, synced_x_ + synced_w_, synced_y_ + synced_h_};
  InvalidateRect(w->hwnd(), &rc, FALSE);
}

void NativeHost::HideGuest() {
  if (synced_visible_ || IsWindowVisible(hwnd_)) {
    ShowWindow(hwnd_, SW_HIDE);
  }
  InvalidateSyncedRect();
  synced_visible_ = false;
}

void NativeHost::SyncNative() {
  if (!attached_) {
    return;
  }
  if (!hwnd_ || !IsWindow(hwnd_)) {
    hwnd_ = nullptr;
    attached_ = false;
    synced_visible_ = false;
    return;
  }
  Window* w = host_window();
  if (!w || !w->hwnd()) {
    HideGuest();
    return;
  }
  AdoptParent();
  const bool show = ShownInTree() && bounds_.w > 0.f && bounds_.h > 0.f;
  if (!show) {
    HideGuest();
    return;
  }
  const float dpi = w->dpi();
  const int x =
      static_cast<int>(std::floor(auralite::PxFromDip(bounds_.x, dpi)));
  const int y =
      static_cast<int>(std::floor(auralite::PxFromDip(bounds_.y, dpi)));
  const int ww = (std::max)(
      0, static_cast<int>(std::ceil(auralite::PxFromDip(bounds_.w, dpi))));
  const int hh = (std::max)(
      0, static_cast<int>(std::ceil(auralite::PxFromDip(bounds_.h, dpi))));
  if (ww <= 0 || hh <= 0) {
    HideGuest();
    return;
  }
  const bool already_shown = synced_visible_ && IsWindowVisible(hwnd_);
  if (already_shown && synced_x_ == x && synced_y_ == y && synced_w_ == ww &&
      synced_h_ == hh) {
    return;
  }
  // SWP_SHOWWINDOW on an already-visible HWND flashes it (hide/show). Chromium
  // NativeViewHost only sets that flag when transitioning hidden → shown.
  // SWP_NOCOPYBITS avoids copying the guest's pixels onto the parent (ghosts).
  if (already_shown) {
    InvalidateSyncedRect();
  }
  UINT flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER |
               SWP_NOCOPYBITS | SWP_DEFERERASE;
  if (!already_shown) {
    flags |= SWP_SHOWWINDOW;
  }
  SetWindowPos(hwnd_, nullptr, x, y, ww, hh, flags);
  synced_x_ = x;
  synced_y_ = y;
  synced_w_ = ww;
  synced_h_ = hh;
  synced_visible_ = true;
}

}  // namespace auralite::ui
