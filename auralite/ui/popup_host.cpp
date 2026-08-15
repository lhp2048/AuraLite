#include "auralite/ui/popup_host.h"

#include "auralite/async/task_lambda.h"
#include "auralite/ui/submenu.h"

#include <cmath>
#include <optional>

namespace auralite::ui {
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
  if (alive_) {
    *alive_ = false;
  }
  dismiss_posted_ = false;
  Dismiss();
}

PopupHost* PopupHost::Current() { return g_current; }

void PopupHost::ClearOpenState() {
  dismiss_posted_ = false;
  dismiss_posted_level_ = 0;
  UninstallMouseHook();
  UninstallOwnerHook();
  owner_ = nullptr;
  if (g_current == this) {
    g_current = nullptr;
  }
}

void PopupHost::Show(HWND owner, POINT screen, std::unique_ptr<Node> root) {
  Dismiss();
  if (!root || !owner) {
    return;
  }
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
  auto root = f.CreateFromYaml(path_or_yaml, handlers);
  if (root) {
    Show(owner, screen, std::move(root));
  }
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
  if (dismiss_posted_) {
    if (level < dismiss_posted_level_) {
      dismiss_posted_level_ = level;
    }
    return;
  }
  dismiss_posted_ = true;
  dismiss_posted_level_ = level;
  MessageLoop* loop = MessageLoop::current();
  if (!loop) {
    dismiss_posted_ = false;
    DismissFrom(level);
    return;
  }
  // Non-nestable: do not run during MessageBox / nested pumps (about dialog).
  auto alive = alive_;
  loop->PostNonNestableTask(new auralite::async::LambdaTask([this, alive] {
    if (!alive || !*alive) {
      return;
    }
    const size_t lvl = dismiss_posted_level_;
    dismiss_posted_ = false;
    dismiss_posted_level_ = 0;
    DismissFrom(lvl);
  }));
}

std::function<void()> PopupHost::WrapDismiss(std::function<void()> fn) {
  return [this, fn = std::move(fn)] {
    if (fn) {
      fn();
    }
    RequestDismiss();
  };
}

SizeF PopupHost::MeasureFit(Node* root) {
  if (!root) {
    return SizeF{120.f, 1.f};
  }
  SizeF s = root->Measure(400.f, 800.f);
  if (s.w < 120.f) {
    s.w = 120.f;
  }
  if (s.h < 1.f) {
    s.h = 1.f;
  }
  return s;
}

void PopupHost::PlaceRoot(Window* w, POINT screen, SizeF content) {
  if (!w || !w->hwnd()) {
    return;
  }
  RectF work = WorkAreaNear(screen);
  POINT tl = ClampTopLeft(screen, content, work);
  SetWindowPos(w->hwnd(), HWND_TOPMOST, tl.x, tl.y,
               static_cast<int>(std::ceil(content.w)),
               static_cast<int>(std::ceil(content.h)), SWP_SHOWWINDOW);
}

void PopupHost::PlaceChild(Window* w, const RectF& anchor, SizeF content) {
  if (!w || !w->hwnd()) {
    return;
  }
  RectF work = WorkAreaNear(
      POINT{static_cast<LONG>(anchor.x), static_cast<LONG>(anchor.y)});
  float x = anchor.x + anchor.w;  // prefer right of item
  if (x + content.w > work.x + work.w) {
    x = anchor.x - content.w;  // flip left
  }
  float y = anchor.y;
  POINT tl =
      ClampTopLeft(POINT{static_cast<LONG>(x), static_cast<LONG>(y)}, content,
                   work);
  SetWindowPos(w->hwnd(), HWND_TOPMOST, tl.x, tl.y,
               static_cast<int>(std::ceil(content.w)),
               static_cast<int>(std::ceil(content.h)), SWP_SHOWWINDOW);
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
  // Deactivate outside stack → dismiss entire stack. Nested Push activates a
  // deeper layer already in stack_; that must not dismiss parents.
  raw->set_on_deactivate_outside([this](HWND activating) {
    // lParam may be NULL when focus is briefly cleared during nested
    // ShowWindow/SetForegroundWindow — do not treat as "outside".
    if (!activating || IsHwndInStack(activating)) {
      return;
    }
    RequestDismiss();
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
  SetPropW(owner_, L"AuraLite.PopupHost", this);
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
    RemovePropW(owner_, L"AuraLite.PopupHost");
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
      // May UnhookWindowsHookEx — CallNextHookEx(nullptr) is still valid.
      g_mouse_hook_host->RequestDismiss();
    }
  }
  return CallNextHookEx(nullptr, code, wparam, lparam);
}

LRESULT CALLBACK PopupHost::OwnerSubclassProc(HWND hwnd, UINT msg,
                                              WPARAM wparam, LPARAM lparam) {
  auto* host =
      reinterpret_cast<PopupHost*>(GetPropW(hwnd, L"AuraLite.PopupHost"));
  WNDPROC old_proc = host ? host->owner_old_proc_ : nullptr;

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

}  // namespace auralite::ui
