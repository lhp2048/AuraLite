#include "mx/ui/popup_host.h"

#include "mx/ui/submenu.h"
#include "mx/ui/yaml_loader.h"

#include <cmath>
#include <optional>

namespace mx::ui {
namespace {

thread_local PopupHost* g_current = nullptr;

// Process-wide LL mouse hook while any PopupHost stack is open (UI thread).
HHOOK g_mouse_hook = nullptr;
PopupHost* g_mouse_hook_host = nullptr;

RectF WorkAreaNear(POINT screen) {
  HMONITOR mon = MonitorFromPoint(screen, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi = {};
  mi.cbSize = sizeof(mi);
  if (!GetMonitorInfoW(mon, &mi)) {
    return RectF{0.f, 0.f, 1920.f, 1080.f};
  }
  const RECT& r = mi.rcWork;
  return RectF{static_cast<float>(r.left), static_cast<float>(r.top),
               static_cast<float>(r.right - r.left),
               static_cast<float>(r.bottom - r.top)};
}

RectF MonitorAreaNear(POINT screen) {
  HMONITOR mon = MonitorFromPoint(screen, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi = {};
  mi.cbSize = sizeof(mi);
  if (!GetMonitorInfoW(mon, &mi)) {
    return RectF{0.f, 0.f, 1920.f, 1080.f};
  }
  const RECT& r = mi.rcMonitor;
  return RectF{static_cast<float>(r.left), static_cast<float>(r.top),
               static_cast<float>(r.right - r.left),
               static_cast<float>(r.bottom - r.top)};
}

POINT ClampTopLeft(POINT desired, SizeF size, const RectF& work) {
  float x = static_cast<float>(desired.x);
  float y = static_cast<float>(desired.y);
  const float w = size.w;
  const float h = size.h;
  if (x + w > work.x + work.w) {
    x = work.x + work.w - w;
  }
  if (y + h > work.y + work.h) {
    y = work.y + work.h - h;
  }
  if (x < work.x) {
    x = work.x;
  }
  if (y < work.y) {
    y = work.y;
  }
  return POINT{static_cast<LONG>(x), static_cast<LONG>(y)};
}

}  // namespace

PopupHost::PopupHost() = default;

PopupHost::~PopupHost() {
  dismiss_pending_ = false;
  after_dismiss_ = nullptr;
  Dismiss();
}

PopupHost* PopupHost::Current() { return g_current; }

void PopupHost::ClearOpenState() {
  dismiss_pending_ = false;
  dismiss_pending_level_ = 0;
  // Do not clear after_dismiss_ here: FlushPendingDismiss may still run it
  // after DismissFrom emptied the stack.
  UninstallMouseHook();
  UninstallOwnerHook();
  owner_ = nullptr;
  show_options_ = PopupShowOptions{};
  if (g_current == this) {
    g_current = nullptr;
  }
}

void PopupHost::Show(HWND owner, POINT screen, std::unique_ptr<Node> root) {
  Show(owner, screen, std::move(root), PopupShowOptions{});
}

void PopupHost::Show(HWND owner, POINT screen, std::unique_ptr<Node> root,
                     PopupShowOptions options) {
  Dismiss();
  if (!root || !owner) {
    return;
  }
  show_options_ = options;
  owner_ = owner;
  g_current = this;
  InstallOwnerHook();
  InstallMouseHook();
  (void)ShowLayer(std::move(root), screen, nullptr);
  // CreatePopup failed (or ShowLayer no-op): do not leave TLS / hooks dangling.
  if (stack_.empty()) {
    ClearOpenState();
  }
}

void PopupHost::ShowFromYaml(HWND owner, POINT screen,
                             const std::string& path_or_yaml,
                             const HandlerMap& handlers) {
  ViewFactory f;
  WindowYaml spec;
  auto root = f.CreateFromYaml(path_or_yaml, handlers, &spec);
  if (!root) {
    return;
  }
  PopupShowOptions opt;
  if (spec.has_corner_radius) {
    opt.corner_radius = spec.options.corner_radius;
  }
  if (spec.has_border_width) {
    opt.border_width = spec.options.border_width;
  }
  opt.theme = spec.theme;
  Show(owner, screen, std::move(root), opt);
}

std::unique_ptr<Node> PopupHost::Push(const RectF& anchor_screen,
                                      std::unique_ptr<Node> root,
                                      Submenu* return_to) {
  if (!root) {
    return nullptr;
  }
  if (stack_.empty()) {
    // Caller (Submenu) must retain leftover; do not destroy.
    return root;
  }
  g_current = this;
  InstallMouseHook();
  const size_t before = stack_.size();
  auto leftover = ShowLayer(std::move(root), POINT{}, &anchor_screen);
  if (leftover) {
    return leftover;
  }
  if (stack_.size() > before) {
    stack_.back().return_to = return_to;
  }
  return nullptr;
}

std::optional<size_t> PopupHost::LevelOf(const Window* window) const {
  for (size_t i = 0; i < stack_.size(); ++i) {
    if (stack_[i].window.get() == window) {
      return i;
    }
  }
  return std::nullopt;
}

void PopupHost::OnPopupHit(Window* window, Node* hit) {
  if (!window || !hit || stack_.size() < 2) {
    return;
  }
  const std::optional<size_t> level = LevelOf(window);
  if (!level) {
    return;
  }
  const size_t child = *level + 1;
  if (child >= stack_.size()) {
    return;
  }
  Submenu* opened_from = stack_[child].return_to;
  if (!opened_from) {
    return;
  }
  for (Node* n = hit; n; n = n->parent()) {
    if (n == opened_from) {
      return;
    }
  }
  DismissFrom(child);
}

void PopupHost::DismissFrom(size_t level) {
  while (stack_.size() > level) {
    Layer& layer = stack_.back();
    if (layer.window) {
      // Avoid re-entrant Dismiss via WM_ACTIVATE while destroying.
      layer.window->set_on_deactivate_outside({});
      if (layer.return_to) {
        auto root = layer.window->ReleaseRoot();
        if (root) {
          layer.return_to->content(std::move(root));
        }
      }
    }
    stack_.pop_back();
  }
  if (stack_.empty()) {
    ClearOpenState();
  }
}

void PopupHost::Dismiss() { DismissFrom(0); }

void PopupHost::RequestDismiss() { RequestDismissFrom(0); }

void PopupHost::RequestDismissFrom(size_t level) {
  if (dismiss_pending_) {
    if (level < dismiss_pending_level_) {
      dismiss_pending_level_ = level;
    }
    return;
  }
  dismiss_pending_ = true;
  dismiss_pending_level_ = level;
}

bool PopupHost::FlushPendingDismiss() {
  if (!dismiss_pending_) {
    return false;
  }
  const size_t lvl = dismiss_pending_level_;
  dismiss_pending_ = false;
  dismiss_pending_level_ = 0;
  auto after = std::move(after_dismiss_);
  after_dismiss_ = nullptr;
  DismissFrom(lvl);
  if (after) {
    after();
  }
  return true;
}

std::function<void()> PopupHost::WrapDismiss(std::function<void()> fn) {
  // Close menu first (flushed after DispatchMouse), then run |fn| (MessageBox).
  return [this, fn = std::move(fn)] {
    if (fn) {
      after_dismiss_ = std::move(fn);
    }
    RequestDismiss();
  };
}

SizeF PopupHost::MeasureFit(Node* root) {
  if (!root) {
    return SizeF{72.f, 1.f};
  }
  std::string theme = show_options_.theme;
  if (theme.empty()) {
    if (Window* owner_ui = Window::FromHwnd(owner_)) {
      theme = owner_ui->theme_name();
    }
  }
  Theme::Scope theme_scope(std::move(theme));
  // Popup HWND size hugs content. A Fill root would otherwise expand to the
  // 400 DIP measure cap (MenuBar Column + Fill buttons).
  const SizePolicy saved_w = root->width_policy();
  const SizePolicy saved_h = root->height_policy();
  if (saved_w == SizePolicy::Fill) {
    root->hug_width();
  }
  if (saved_h == SizePolicy::Fill) {
    root->hug_height();
  }
  SizeF s = root->Measure(400.f, 800.f);
  root->set_width_policy(saved_w);
  root->set_height_policy(saved_h);
  if (s.w < 72.f) {
    s.w = 72.f;
  }
  if (s.h < 1.f) {
    s.h = 1.f;
  }
  return s;
}

void PopupHost::Replace(std::unique_ptr<Node> root, POINT screen) {
  if (!root) {
    return;
  }
  if (stack_.empty() || !owner_) {
    Show(owner_, screen, std::move(root), show_options_);
    return;
  }
  DismissFrom(1);
  if (stack_.empty() || !stack_[0].window) {
    Show(owner_, screen, std::move(root), show_options_);
    return;
  }
  SizeF content = MeasureFit(root.get());
  Window* raw = stack_[0].window.get();
  raw->SetPopupChrome(show_options_.corner_radius, show_options_.border_width);
  raw->SetRoot(std::move(root));
  PlaceRoot(raw, screen, content, false);
}

void PopupHost::PlaceRoot(Window* w, POINT screen, SizeF content, bool activate) {
  if (!w || !w->hwnd()) {
    return;
  }
  const float dpi = w->dpi();
  const SizeF px{mx::PxFromDip(content.w, dpi),
                 mx::PxFromDip(content.h, dpi)};
  const RectF area = show_options_.clamp_to_monitor ? MonitorAreaNear(screen)
                                                    : WorkAreaNear(screen);
  POINT desired = screen;
  if (show_options_.placement == PopupPlacement::kBottomLeftAtPoint) {
    desired.y -= static_cast<LONG>(std::ceil(px.h));
  }
  POINT tl = ClampTopLeft(desired, px, area);
  UINT flags = SWP_SHOWWINDOW;
  if (!activate) {
    flags |= SWP_NOACTIVATE;
  }
  SetWindowPos(w->hwnd(), HWND_TOPMOST, tl.x, tl.y,
               static_cast<int>(std::ceil(px.w)),
               static_cast<int>(std::ceil(px.h)), flags);
}

void PopupHost::PlaceChild(Window* w, const RectF& anchor, SizeF content) {
  if (!w || !w->hwnd()) {
    return;
  }
  const float dpi = w->dpi();
  const SizeF px{mx::PxFromDip(content.w, dpi),
                 mx::PxFromDip(content.h, dpi)};
  RectF work = WorkAreaNear(
      POINT{static_cast<LONG>(anchor.x), static_cast<LONG>(anchor.y)});
  float x = anchor.x + anchor.w;  // prefer right of item
  if (x + px.w > work.x + work.w) {
    x = anchor.x - px.w;  // flip left
  }
  float y = anchor.y;
  POINT tl =
      ClampTopLeft(POINT{static_cast<LONG>(x), static_cast<LONG>(y)}, px, work);
  SetWindowPos(w->hwnd(), HWND_TOPMOST, tl.x, tl.y,
               static_cast<int>(std::ceil(px.w)),
               static_cast<int>(std::ceil(px.h)), SWP_SHOWWINDOW);
}

std::unique_ptr<Node> PopupHost::ShowLayer(std::unique_ptr<Node> root,
                                           POINT screen_or_ignored,
                                           const RectF* anchor_opt) {
  if (!root || !owner_) {
    return root;
  }

  SizeF content = MeasureFit(root.get());
  const int cw = static_cast<int>(std::ceil(content.w));
  const int ch = static_cast<int>(std::ceil(content.h));

  auto window = std::make_unique<Window>();
  if (!window->CreatePopup(owner_, cw, ch)) {
    return root;
  }

  Window* raw = window.get();
  if (!show_options_.theme.empty()) {
    raw->set_theme(show_options_.theme);
  }
  raw->SetPopupChrome(show_options_.corner_radius, show_options_.border_width);
  // Deactivate outside stack → dismiss entire stack. Nested Push activates a
  // deeper layer already in stack_; that must not dismiss parents.
  raw->set_on_deactivate_outside([this](HWND activating) {
    // lParam may be NULL when focus is briefly cleared during nested
    // ShowWindow/SetForegroundWindow — do not treat as "outside".
    if (!activating || IsHwndInStack(activating)) {
      return;
    }
    RequestDismiss();
    if (owner_) {
      PostMessageW(owner_, WM_APP + 0x414C, 0, 0);
    }
  });

  raw->SetRoot(std::move(root));

  // Push before show/activate so WA_INACTIVE on the parent sees the new HWND
  // as in-stack.
  stack_.push_back(Layer{std::move(window)});
  g_current = this;

  if (anchor_opt) {
    PlaceChild(raw, *anchor_opt, content);
  } else {
    PlaceRoot(raw, screen_or_ignored, content);
  }

  ShowWindow(raw->hwnd(), SW_SHOW);
  SetForegroundWindow(raw->hwnd());
  SetFocus(raw->hwnd());
  return nullptr;
}

bool PopupHost::HitAnyPopup(POINT screen) const {
  POINT pt = screen;
  for (const auto& layer : stack_) {
    if (!layer.window || !layer.window->hwnd()) {
      continue;
    }
    RECT rc = {};
    if (!GetWindowRect(layer.window->hwnd(), &rc)) {
      continue;
    }
    if (PtInRect(&rc, pt)) {
      return true;
    }
  }
  return false;
}

bool PopupHost::IsHwndInStack(HWND hwnd) const {
  if (!hwnd) {
    return false;
  }
  for (const auto& layer : stack_) {
    HWND ph = layer.window ? layer.window->hwnd() : nullptr;
    if (!ph) {
      continue;
    }
    if (hwnd == ph || IsChild(ph, hwnd)) {
      return true;
    }
  }
  return false;
}

void PopupHost::InstallOwnerHook() {
  if (owner_hooked_ || !owner_ || !IsWindow(owner_)) {
    return;
  }
  owner_old_proc_ = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
      owner_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&OwnerSubclassProc)));
  if (!owner_old_proc_) {
    return;
  }
  SetPropW(owner_, L"MxUI.PopupHost", this);
  owner_hooked_ = true;
}

void PopupHost::UninstallOwnerHook() {
  if (!owner_hooked_ || !owner_) {
    owner_hooked_ = false;
    owner_old_proc_ = nullptr;
    return;
  }
  if (IsWindow(owner_) && owner_old_proc_) {
    SetWindowLongPtrW(owner_, GWLP_WNDPROC,
                      reinterpret_cast<LONG_PTR>(owner_old_proc_));
  }
  if (IsWindow(owner_)) {
    RemovePropW(owner_, L"MxUI.PopupHost");
  }
  owner_hooked_ = false;
  owner_old_proc_ = nullptr;
}

void PopupHost::InstallMouseHook() {
  if (mouse_hooked_ || g_mouse_hook) {
    mouse_hooked_ = (g_mouse_hook_host == this);
    return;
  }
  // WH_MOUSE_LL: temporary while stack open; catches outside clicks that do
  // not reliably produce WM_ACTIVATE on the popup (e.g. click on owner).
  g_mouse_hook = SetWindowsHookExW(WH_MOUSE_LL, &MouseHookProc,
                                   GetModuleHandleW(nullptr), 0);
  if (!g_mouse_hook) {
    return;
  }
  g_mouse_hook_host = this;
  mouse_hooked_ = true;
}

void PopupHost::UninstallMouseHook() {
  if (g_mouse_hook && g_mouse_hook_host == this) {
    UnhookWindowsHookEx(g_mouse_hook);
    g_mouse_hook = nullptr;
    g_mouse_hook_host = nullptr;
  }
  mouse_hooked_ = false;
}

LRESULT CALLBACK PopupHost::MouseHookProc(int code, WPARAM wparam,
                                          LPARAM lparam) {
  if (code == HC_ACTION && g_mouse_hook_host &&
      (wparam == WM_LBUTTONDOWN || wparam == WM_RBUTTONDOWN)) {
    const auto* info = reinterpret_cast<const MSLLHOOKSTRUCT*>(lparam);
    if (info && !g_mouse_hook_host->HitAnyPopup(info->pt)) {
      // Keep activate path; this covers cases activate misses.
      g_mouse_hook_host->RequestDismiss();
      // Not inside a popup DispatchMouse — flush via owner after hook returns.
      if (g_mouse_hook_host->owner_) {
        PostMessageW(g_mouse_hook_host->owner_, WM_APP + 0x414C, 0, 0);
      }
    }
  }
  return CallNextHookEx(nullptr, code, wparam, lparam);
}

LRESULT CALLBACK PopupHost::OwnerSubclassProc(HWND hwnd, UINT msg,
                                              WPARAM wparam, LPARAM lparam) {
  auto* host =
      reinterpret_cast<PopupHost*>(GetPropW(hwnd, L"MxUI.PopupHost"));
  WNDPROC old_proc = host ? host->owner_old_proc_ : nullptr;

  if (msg == WM_APP + 0x414C && host) {
    host->FlushPendingDismiss();
    return 0;
  }

  if (msg == WM_DESTROY && host) {
    // Owner going away: drop stack before default destroy tears owned HWNDs.
    host->UninstallOwnerHook();
    host->Dismiss();
    if (old_proc) {
      return CallWindowProcW(old_proc, hwnd, msg, wparam, lparam);
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
  }

  if (old_proc) {
    return CallWindowProcW(old_proc, hwnd, msg, wparam, lparam);
  }
  return DefWindowProcW(hwnd, msg, wparam, lparam);
}

}  // namespace mx::ui
