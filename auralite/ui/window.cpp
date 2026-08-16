#include "auralite/ui/window.h"

#include "auralite/async/task_lambda.h"
#include "auralite/ui/label.h"
#include "auralite/ui/popup_host.h"
#include "auralite/ui/theme.h"
#include "message_framework/message_loop.h"

#include <imm.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace auralite::ui {
namespace {

constexpr wchar_t kWindowClassName[] = L"AuraLite.UI.Window";

float QueryMonitorDpiNearCursor() {
  using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
  static GetDpiForMonitorFn fn = nullptr;
  static bool tried = false;
  if (!tried) {
    tried = true;
    HMODULE shcore = LoadLibraryW(L"shcore.dll");
    if (shcore) {
      fn = reinterpret_cast<GetDpiForMonitorFn>(
          GetProcAddress(shcore, "GetDpiForMonitor"));
    }
  }
  POINT pt = {};
  GetCursorPos(&pt);
  HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
  UINT dx = 96;
  UINT dy = 96;
  // MDT_EFFECTIVE_DPI = 0
  if (fn && mon && SUCCEEDED(fn(mon, 0, &dx, &dy)) && dx > 0) {
    return static_cast<float>(dx);
  }
  return auralite::kDipDpi;
}

float QueryHwndDpi(HWND hwnd) {
  using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
  static GetDpiForWindowFn fn = nullptr;
  static bool tried = false;
  if (!tried) {
    tried = true;
    fn = reinterpret_cast<GetDpiForWindowFn>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
  }
  if (fn && hwnd) {
    const UINT d = fn(hwnd);
    if (d > 0) {
      return static_cast<float>(d);
    }
  }
  return auralite::kDipDpi;
}

int DipToOuterPx(float dip, float dpi) {
  return static_cast<int>(std::ceil(auralite::PxFromDip(dip, dpi)));
}

bool IsImeUiWindow(HWND hwnd, HWND dialog) {
  if (!hwnd) {
    return false;
  }
  wchar_t cls[128] = {};
  GetClassNameW(hwnd, cls, 128);
  if (wcsstr(cls, L"IME") != nullptr || wcsstr(cls, L"MSCTFIME") != nullptr) {
    return true;
  }
  return GetWindow(hwnd, GW_OWNER) == dialog;
}

class ModalDispatcher : public MessageLoopForUI::Dispatcher {
 public:
  explicit ModalDispatcher(HWND focus) : focus_(focus) {}
  bool Dispatch(const MSG& msg) override {
    if (focus_ && IsWindow(focus_)) {
      const HWND focus = GetFocus();
      if (focus != focus_ && !IsChild(focus_, focus) &&
          !IsImeUiWindow(focus, focus_)) {
        if (GetForegroundWindow() != focus_) {
          SetForegroundWindow(focus_);
        }
        SetActiveWindow(focus_);
        SetFocus(focus_);
      }
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    return true;
  }

 private:
  HWND focus_;
};

std::wstring ImmGetString(HIMC himc, DWORD index) {
  if (!himc) {
    return {};
  }
  const LONG bytes = ImmGetCompositionStringW(himc, index, nullptr, 0);
  if (bytes <= 0) {
    return {};
  }
  std::wstring out(static_cast<size_t>(bytes) / sizeof(wchar_t), L'\0');
  ImmGetCompositionStringW(himc, index, out.data(),
                           static_cast<DWORD>(bytes));
  // Imm may not null-terminate; size already matches char count.
  while (!out.empty() && out.back() == L'\0') {
    out.pop_back();
  }
  return out;
}

}  // namespace

Window::Window() = default;

Window::~Window() {
  if (alive_) {
    alive_->store(false);
  }
  Theme::RemoveInvalidateSink(&theme_sink_);
  HideTooltip();
  tooltip_window_.reset();
  if (hwnd_) {
    if (anim_clients_ > 0) {
      KillTimer(hwnd_, kAnimTimerId);
      anim_clients_ = 0;
    }
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
    popup_mode_ = false;
  }
}

void Window::RegisterAnimation() {
  ++anim_clients_;
  if (anim_clients_ == 1 && hwnd_) {
    SetTimer(hwnd_, kAnimTimerId, 33, nullptr);
  }
}

void Window::UnregisterAnimation() {
  if (anim_clients_ <= 0) {
    return;
  }
  --anim_clients_;
  if (anim_clients_ == 0 && hwnd_) {
    KillTimer(hwnd_, kAnimTimerId);
  }
}

bool Window::EnsureWindowClass(HINSTANCE instance) {
  static bool registered = false;
  if (registered) {
    return true;
  }

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = &Window::WndProc;
  wc.hInstance = instance;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = nullptr;
  wc.lpszClassName = kWindowClassName;
  if (!RegisterClassExW(&wc)) {
    if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
      return false;
    }
  }
  registered = true;
  return true;
}

bool Window::Create(const wchar_t* title, int w, int h) {
  if (hwnd_) {
    return false;
  }

  (void)Theme::Active();

  HINSTANCE instance = GetModuleHandleW(nullptr);
  if (!EnsureWindowClass(instance)) {
    return false;
  }

  dpi_ = QueryMonitorDpiNearCursor();
  const int pw = DipToOuterPx(static_cast<float>(w), dpi_);
  const int ph = DipToOuterPx(static_cast<float>(h), dpi_);

  hwnd_ = CreateWindowExW(
      0, kWindowClassName, title ? title : L"AuraLite", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, pw, ph, nullptr, nullptr, instance, this);
  if (!hwnd_) {
    return false;
  }

  dpi_ = QueryHwndDpi(hwnd_);
  canvas_.SetDpi(dpi_);

  if (!canvas_.Init(hwnd_)) {
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
    return false;
  }

  // Detach default IME until a WantsIme() node receives focus.
  ImmAssociateContextEx(hwnd_, NULL, 0);

  theme_sink_ = [this] { Invalidate(); };
  Theme::AddInvalidateSink(&theme_sink_);

  layout_dirty_ = true;
  return true;
}

bool Window::CreatePopup(HWND owner, int w, int h) {
  if (hwnd_) {
    return false;
  }

  (void)Theme::Active();

  HINSTANCE instance = GetModuleHandleW(nullptr);
  if (!EnsureWindowClass(instance)) {
    return false;
  }

  popup_mode_ = true;
  quit_on_close_ = false;

  dpi_ = QueryMonitorDpiNearCursor();
  const int pw = DipToOuterPx(static_cast<float>(w), dpi_);
  const int ph = DipToOuterPx(static_cast<float>(h), dpi_);

  hwnd_ = CreateWindowExW(
      WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED, kWindowClassName, L"",
      WS_POPUP | WS_CLIPCHILDREN, 0, 0, pw, ph, owner, nullptr, instance,
      this);
  if (!hwnd_) {
    popup_mode_ = false;
    quit_on_close_ = true;
    return false;
  }

  dpi_ = QueryHwndDpi(hwnd_);
  canvas_.SetDpi(dpi_);

  if (!canvas_.InitLayered(hwnd_)) {
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
    popup_mode_ = false;
    quit_on_close_ = true;
    return false;
  }

  ImmAssociateContextEx(hwnd_, NULL, 0);

  theme_sink_ = [this] { Invalidate(); };
  Theme::AddInvalidateSink(&theme_sink_);

  layout_dirty_ = true;
  return true;
}

bool Window::CreateDialogWindow(HWND owner, int width, int height,
                                const DialogOptions& opt) {
  return CreateDialogWindow(owner, L"", width, height, opt);
}

bool Window::CreateDialogWindow(HWND owner, const wchar_t* title, int width,
                                int height, const DialogOptions& opt) {
  if (hwnd_) {
    return false;
  }

  (void)Theme::Active();

  HINSTANCE instance = GetModuleHandleW(nullptr);
  if (!EnsureWindowClass(instance)) {
    return false;
  }

  dialog_mode_ = true;
  quit_on_close_ = false;
  popup_mode_ = false;
  dialog_owner_ = owner;
  dialog_opt_ = opt;

  dpi_ = QueryMonitorDpiNearCursor();
  const int pw = DipToOuterPx(static_cast<float>(width), dpi_);
  const int ph = DipToOuterPx(static_cast<float>(height), dpi_);

  DWORD ex = 0;
  if (opt.topmost) {
    ex |= WS_EX_TOPMOST;
  }

  hwnd_ = CreateWindowExW(ex, kWindowClassName, title ? title : L"",
                          WS_POPUP | WS_CLIPCHILDREN, 0, 0, pw, ph, owner,
                          nullptr, instance, this);
  if (!hwnd_) {
    dialog_mode_ = false;
    quit_on_close_ = true;
    dialog_owner_ = nullptr;
    return false;
  }

  dpi_ = QueryHwndDpi(hwnd_);
  canvas_.SetDpi(dpi_);

  if (!canvas_.Init(hwnd_)) {
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
    dialog_mode_ = false;
    quit_on_close_ = true;
    dialog_owner_ = nullptr;
    return false;
  }

  ImmAssociateContextEx(hwnd_, NULL, 0);

  theme_sink_ = [this] { Invalidate(); };
  Theme::AddInvalidateSink(&theme_sink_);

  layout_dirty_ = true;
  PlaceDialogWindow(owner, width, height, opt);
  return true;
}

void Window::PlaceDialogWindow(HWND owner, int width_dip, int height_dip,
                               const DialogOptions& opt) {
  if (!hwnd_) {
    return;
  }

  const int pw = DipToOuterPx(static_cast<float>(width_dip), dpi_);
  const int ph = DipToOuterPx(static_cast<float>(height_dip), dpi_);

  int x = 0;
  int y = 0;
  HMONITOR mon = nullptr;
  const bool center_owner =
      opt.center_on_owner && owner && IsWindow(owner);

  if (center_owner) {
    RECT orc = {};
    GetWindowRect(owner, &orc);
    x = orc.left + (orc.right - orc.left - pw) / 2;
    y = orc.top + (orc.bottom - orc.top - ph) / 2;
    mon = MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST);
  } else {
    POINT pt = {};
    GetCursorPos(&pt);
    mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
  }

  MONITORINFO mi = {};
  mi.cbSize = sizeof(mi);
  if (mon && GetMonitorInfoW(mon, &mi)) {
    const RECT& work = mi.rcWork;
    if (!center_owner) {
      x = work.left + (work.right - work.left - pw) / 2;
      y = work.top + (work.bottom - work.top - ph) / 2;
    }
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

  SetWindowPos(hwnd_, opt.topmost ? HWND_TOPMOST : HWND_TOP, x, y, pw, ph,
               SWP_FRAMECHANGED);
}

void Window::ActivateDialogHwnd() {
  if (!hwnd_) {
    return;
  }
  AllowSetForegroundWindow(ASFW_ANY);
  ShowWindow(hwnd_, SW_SHOWNORMAL);
  HWND fg = GetForegroundWindow();
  DWORD fg_tid = fg ? GetWindowThreadProcessId(fg, nullptr) : 0;
  const DWORD cur_tid = GetCurrentThreadId();
  const bool attached =
      (fg_tid != 0 && fg_tid != cur_tid) &&
      AttachThreadInput(cur_tid, fg_tid, TRUE);
  SetWindowPos(hwnd_, dialog_opt_.topmost ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
  BringWindowToTop(hwnd_);
  SetForegroundWindow(hwnd_);
  SetActiveWindow(hwnd_);
  SetFocus(hwnd_);
  if (attached) {
    AttachThreadInput(cur_tid, fg_tid, FALSE);
  }
}

void Window::RestoreDialogOwner() {
  if (dialog_owner_ && IsWindow(dialog_owner_)) {
    EnableWindow(dialog_owner_, TRUE);
  }
}

int Window::RunModal() {
  if (!hwnd_ || !dialog_mode_) {
    return IDABORT;
  }
  MessageLoop* base_loop = MessageLoop::current();
  if (!base_loop || base_loop->type() != MessageLoop::TYPE_UI) {
    return IDABORT;
  }
  MessageLoopForUI* loop = MessageLoopForUI::current();
  if (!loop) {
    return IDABORT;
  }
  HideTooltip();
  if (dialog_owner_) {
    if (Window* owner_ui = FromHwnd(dialog_owner_)) {
      owner_ui->HideTooltip();
    }
  }
  modal_result_ = IDCANCEL;
  modal_running_ = true;
  if (dialog_owner_ && IsWindow(dialog_owner_)) {
    EnableWindow(dialog_owner_, FALSE);
  }
  ActivateDialogHwnd();
  ModalDispatcher dispatcher(hwnd_);
  loop->Run(&dispatcher);
  modal_running_ = false;
  RestoreDialogOwner();
  if (hwnd_) {
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
  }
  return modal_result_;
}

void Window::EndModal(int result) {
  modal_result_ = result;
  HideTooltip();
  if (modal_running_ && MessageLoop::current()) {
    MessageLoop::current()->Quit();
  }
  if (hwnd_ && !modal_running_) {
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
  }
}

void Window::SetRoot(std::unique_ptr<Node> root) {
  ClearPopup();
  SetFocusNode(nullptr);
  root_ = std::move(root);
  if (root_) {
    root_->set_host_window(this);
  }
  mouse_capture_ = nullptr;
  hovered_ = nullptr;
  layout_dirty_ = true;
  Invalidate();
}

std::unique_ptr<Node> Window::ReleaseRoot() {
  ClearPopup();
  SetFocusNode(nullptr);
  mouse_capture_ = nullptr;
  hovered_ = nullptr;
  if (root_) {
    root_->set_host_window(nullptr);
  }
  layout_dirty_ = true;
  return std::move(root_);
}

void Window::SetPopup(std::unique_ptr<Node> popup,
                      std::function<void()> on_dismiss, Node* anchor) {
  popup_ = std::move(popup);
  if (popup_) {
    popup_->set_host_window(this);
  }
  popup_dismiss_ = std::move(on_dismiss);
  popup_anchor_ = anchor;
  Invalidate();
}

void Window::ClearPopup() {
  clear_popup_pending_ = false;
  if (popup_) {
    if (focused_ == popup_.get()) {
      SetFocusNode(nullptr);
    }
    if (hovered_ == popup_.get()) {
      hovered_ = nullptr;
    }
    if (mouse_capture_ == popup_.get()) {
      mouse_capture_ = nullptr;
      if (hwnd_ && GetCapture() == hwnd_) {
        ReleaseCapture();
      }
    }
  }
  popup_.reset();
  popup_anchor_ = nullptr;
  if (popup_dismiss_) {
    auto dismiss = std::move(popup_dismiss_);
    popup_dismiss_ = nullptr;
    dismiss();
  }
  Invalidate();
}

void Window::RequestClearPopup() {
  if (popup_ || popup_dismiss_) {
    clear_popup_pending_ = true;
  }
}

void Window::SyncPopupLayout() {
  if (!popup_ || !popup_anchor_) {
    return;
  }
  const RectF a = popup_anchor_->bounds();
  const RectF client = ClientRectF();
  // Dismiss when the anchor has scrolled/clipped fully out of the client.
  const bool visible =
      a.x < client.x + client.w && a.x + a.w > client.x &&
      a.y < client.y + client.h && a.y + a.h > client.y;
  if (!visible) {
    RequestClearPopup();
    return;
  }

  float h = popup_->bounds().h;
  if (h <= 0.f) {
    h = popup_->Measure(a.w, 200.f).h;
  }
  if (h <= 0.f) {
    h = 120.f;
  }
  float y = a.y + a.h + 2.f;
  if (y + h > client.y + client.h && a.y - 2.f - h >= client.y) {
    y = a.y - 2.f - h;
  }
  popup_->Layout(RectF{a.x, y, a.w, h});
}

void Window::Invalidate() {
  if (!hwnd_) {
    return;
  }
  // InvalidateRect immediately. Do not PostTask-coalesce: nestable tasks can
  // still be deferred relative to the current WM_* handler, and nested modal
  // MessageLoop runs (password/settings) then show stale UI until the dialog
  // closes — typing works (Enter submits) but bullets never appear.
  invalidate_posted_ = false;
  InvalidateRect(hwnd_, nullptr, FALSE);
}

void Window::RequestLayout() {
  layout_dirty_ = true;
  Invalidate();
}

void Window::CollectFocusable(Node* node, std::vector<Node*>* out) {
  if (!node || !out || !node->visible()) {
    return;
  }
  if (node->focusable()) {
    out->push_back(node);
  }
  for (const auto& child : node->children()) {
    if (child) {
      CollectFocusable(child.get(), out);
    }
  }
}

void Window::SetFocusNode(Node* node) {
  if (focused_ == node) {
    return;
  }
  if (node && !node->focusable()) {
    return;
  }
  if (focused_) {
    focused_->set_focused(false);
    focused_->OnBlur();
  }
  focused_ = node;
  ime_char_suppress_ = 0;
  if (focused_) {
    focused_->set_focused(true);
    focused_->OnFocus();
    if (hwnd_) {
      ::SetFocus(hwnd_);
    }
  }
  UpdateImeAssociation();
  Invalidate();
}

void Window::FocusNext(bool reverse) {
  if (!root_) {
    return;
  }
  std::vector<Node*> list;
  CollectFocusable(root_.get(), &list);
  if (list.empty()) {
    SetFocusNode(nullptr);
    return;
  }

  int index = -1;
  for (size_t i = 0; i < list.size(); ++i) {
    if (list[i] == focused_) {
      index = static_cast<int>(i);
      break;
    }
  }

  if (index < 0) {
    SetFocusNode(reverse ? list.back() : list.front());
    return;
  }

  const int n = static_cast<int>(list.size());
  const int next = reverse ? (index - 1 + n) % n : (index + 1) % n;
  SetFocusNode(list[static_cast<size_t>(next)]);
}

void Window::NotifyDeviceLost() {
  if (root_) {
    root_->OnDeviceLost();
  }
  if (popup_) {
    popup_->OnDeviceLost();
  }
}

void Window::ClearHover() {
  HideTooltip();
  if (!hovered_) {
    return;
  }
  MouseEvent ev;
  hovered_->OnMouseLeave(ev);
  hovered_ = nullptr;
  Invalidate();
}

void Window::EnsureMouseLeaveTracking() {
  if (tracking_mouse_leave_ || !hwnd_) {
    return;
  }
  TRACKMOUSEEVENT tme = {};
  tme.cbSize = sizeof(tme);
  tme.dwFlags = TME_LEAVE;
  tme.hwndTrack = hwnd_;
  if (TrackMouseEvent(&tme)) {
    tracking_mouse_leave_ = true;
  }
}

void Window::UpdateImeAssociation() {
  if (!hwnd_) {
    return;
  }
  if (focused_ && focused_->WantsIme()) {
    ImmAssociateContextEx(hwnd_, NULL, IACE_DEFAULT);
    UpdateImeCandidatePos();
  } else {
    ImmAssociateContextEx(hwnd_, NULL, 0);
  }
}

void Window::UpdateImeCandidatePos() {
  if (!hwnd_ || !focused_ || !focused_->WantsIme()) {
    return;
  }
  HIMC himc = ImmGetContext(hwnd_);
  if (!himc) {
    return;
  }
  const RectF b = focused_->bounds();
  COMPOSITIONFORM form = {};
  form.dwStyle = CFS_POINT;
  form.ptCurrentPos.x =
      static_cast<LONG>(auralite::PxFromDip(b.x + 10.f, dpi_));
  form.ptCurrentPos.y =
      static_cast<LONG>(auralite::PxFromDip(b.y + b.h, dpi_));
  ImmSetCompositionWindow(himc, &form);
  ImmReleaseContext(hwnd_, himc);
}

Window* Window::FromHwnd(HWND hwnd) {
  return reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                 LPARAM lparam) {
  if (msg == WM_NCCREATE) {
    auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
    auto* self = static_cast<Window*>(cs->lpCreateParams);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    self->hwnd_ = hwnd;
    return DefWindowProcW(hwnd, msg, wparam, lparam);
  }

  Window* self = FromHwnd(hwnd);
  if (!self) {
    return DefWindowProcW(hwnd, msg, wparam, lparam);
  }
  return self->HandleMessage(msg, wparam, lparam);
}

LRESULT Window::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_SIZE:
      if (wparam != SIZE_MINIMIZED) {
        OnSize(LOWORD(lparam), HIWORD(lparam));
      }
      return 0;

    case WM_DPICHANGED: {
      const UINT new_dpi = LOWORD(wparam);
      const RECT* rec = reinterpret_cast<const RECT*>(lparam);
      ApplyDpiChange(new_dpi, rec);
      return 0;
    }

    case WM_DISPLAYCHANGE:
    case WM_PAINT: {
      PAINTSTRUCT ps = {};
      BeginPaint(hwnd_, &ps);
      OnPaint();
      EndPaint(hwnd_, &ps);
      return 0;
    }

    case WM_ERASEBKGND:
      return 1;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
      DispatchMouse(msg, wparam, lparam);
      return 0;

    case WM_RBUTTONUP: {
      // We consume mouse messages (no DefWindowProc), so WM_CONTEXTMENU is
      // never synthesized — fire it ourselves from the click point.
      DispatchMouse(msg, wparam, lparam);
      POINT pt = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
      ClientToScreen(hwnd_, &pt);
      DispatchContextMenu(0, MAKELPARAM(pt.x, pt.y));
      return 0;
    }

    case WM_MOUSELEAVE:
      tracking_mouse_leave_ = false;
      ClearHover();
      return 0;

    case WM_CONTEXTMENU:
      DispatchContextMenu(wparam, lparam);
      return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      if (wparam == VK_TAB) {
        const bool reverse = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        FocusNext(reverse);
        return 0;
      }
      DispatchKey(msg, wparam);
      // Esc may have destroyed this Window via PopupHost — do not touch
      // members after DispatchKey when popup_mode_ was set.
      return 0;

    case WM_KEYUP:
    case WM_SYSKEYUP:
      DispatchKey(msg, wparam);
      return 0;

    case WM_CHAR:
      DispatchChar(wparam);
      return 0;

    case WM_IME_STARTCOMPOSITION:
      UpdateImeCandidatePos();
      return 0;

    case WM_IME_COMPOSITION:
      HandleImeComposition(lparam);
      // Let DefWindowProc also run when we want candidate UI; we consume string.
      return 0;

    case WM_IME_ENDCOMPOSITION:
      if (focused_) {
        focused_->OnImeEnd();
        Invalidate();
      }
      return 0;

    case WM_IME_CHAR:
      DispatchImeChar(wparam);
      return 0;

    case WM_ACTIVATE:
      // PopupHost menus: inactive + activation target outside stack → dismiss
      // entire stack. Nested Push activates another layer in-stack — keep open.
      // Non-popup windows must fall through to DefWindowProc.
      if (popup_mode_) {
        if (LOWORD(wparam) == WA_INACTIVE && on_deactivate_outside_) {
          HWND other = reinterpret_cast<HWND>(lparam);
          auto cb = on_deactivate_outside_;
          cb(other);
          // |this| may be destroyed if the host dismissed the stack.
        }
        return 0;
      }
      if (LOWORD(wparam) == WA_INACTIVE) {
        HideTooltip();
      }
      break;

    case WM_SETFOCUS:
      Invalidate();
      return 0;

    case WM_KILLFOCUS:
      Invalidate();
      return 0;

    case WM_TIMER:
      if (wparam == kAnimTimerId) {
        Invalidate();
        return 0;
      }
      if (wparam == kTooltipTimerId) {
        KillTimer(hwnd_, kTooltipTimerId);
        ShowTooltipFor(hovered_);
        return 0;
      }
      break;

    case WM_DESTROY:
      HideTooltip();
      if (dialog_mode_ && modal_running_ && MessageLoop::current()) {
        MessageLoop::current()->Quit();
      }
      if (anim_clients_ > 0) {
        KillTimer(hwnd_, kAnimTimerId);
        anim_clients_ = 0;
      }
      canvas_.Shutdown();
      hwnd_ = nullptr;
      popup_mode_ = false;
      dialog_mode_ = false;
      mouse_capture_ = nullptr;
      hovered_ = nullptr;
      focused_ = nullptr;
      tracking_mouse_leave_ = false;
      // Optional: Application::Run / single-window demos quit here.
      // Embedded hosts (Shell MessageLoop) must set_quit_on_close(false).
      if (quit_on_close_) {
        PostQuitMessage(0);
      }
      return 0;

    default:
      break;
  }
  return DefWindowProcW(hwnd_, msg, wparam, lparam);
}

void Window::HandleImeComposition(LPARAM lparam) {
  if (!focused_ || !focused_->WantsIme()) {
    return;
  }
  HIMC himc = ImmGetContext(hwnd_);
  if (!himc) {
    return;
  }

  if (lparam & GCS_RESULTSTR) {
    const std::wstring result = ImmGetString(himc, GCS_RESULTSTR);
    if (!result.empty()) {
      focused_->OnImeResult(result);
      ime_char_suppress_ += result.size();
    }
  }
  if (lparam & GCS_COMPSTR) {
    focused_->OnImeComposition(ImmGetString(himc, GCS_COMPSTR));
  }

  ImmReleaseContext(hwnd_, himc);
  UpdateImeCandidatePos();
  Invalidate();
}

RectF Window::ClientRectF() const {
  RECT rc = {};
  GetClientRect(hwnd_, &rc);
  return RectF{0.f, 0.f,
               auralite::DipFromPx(static_cast<float>(rc.right), dpi_),
               auralite::DipFromPx(static_cast<float>(rc.bottom), dpi_)};
}

void Window::OnSize(UINT width, UINT height) {
  canvas_.Resize(width, height);
  layout_dirty_ = true;
  Invalidate();
}

void Window::ApplyDpiChange(UINT new_dpi, const RECT* suggested) {
  dpi_ = new_dpi > 0 ? static_cast<float>(new_dpi) : auralite::kDipDpi;
  canvas_.SetDpi(dpi_);
  if (!popup_mode_ && suggested) {
    SetWindowPos(hwnd_, nullptr, suggested->left, suggested->top,
                 suggested->right - suggested->left,
                 suggested->bottom - suggested->top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
  } else if (popup_mode_ && root_) {
    const SizeF dip = root_->Measure(400.f, 800.f);
    SetWindowPos(hwnd_, nullptr, 0, 0, DipToOuterPx(dip.w, dpi_),
                 DipToOuterPx(dip.h, dpi_),
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  }
  RECT rc = {};
  GetClientRect(hwnd_, &rc);
  canvas_.Resize(static_cast<UINT>(rc.right > 0 ? rc.right : 1),
                 static_cast<UINT>(rc.bottom > 0 ? rc.bottom : 1));
  RequestLayout();
  Invalidate();
}

void Window::OnPaint() {
  if (!hwnd_) {
    return;
  }

  const bool need_init = !canvas_.is_valid();
  if (need_init) {
    const bool ok =
        popup_mode_ ? canvas_.InitLayered(hwnd_) : canvas_.Init(hwnd_);
    if (!ok) {
      return;
    }
  }
  if (need_init) {
    NotifyDeviceLost();
  }

  if (!canvas_.BeginDraw()) {
    return;
  }

  if (popup_mode_) {
    canvas_.Clear(ColorF(0.f, 0.f, 0.f, 0.f));
  } else {
    canvas_.Clear(Theme::Active().window_bg);
  }

  if (root_) {
    const RectF client = ClientRectF();
    if (layout_dirty_) {
      root_->Layout(client);
      layout_dirty_ = false;
    }
    root_->Paint(canvas_);
  }
  if (popup_) {
    SyncPopupLayout();
    popup_->Paint(canvas_);
  }

  if (!canvas_.EndDraw()) {
    const bool ok =
        popup_mode_ ? canvas_.InitLayered(hwnd_) : canvas_.Init(hwnd_);
    if (ok) {
      NotifyDeviceLost();
    }
    layout_dirty_ = true;
    Invalidate();
  }
}

MouseButton Window::ButtonFromMsg(UINT msg, WPARAM wparam) {
  switch (msg) {
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
      return MouseButton::Right;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
      return MouseButton::Middle;
    case WM_MOUSEWHEEL:
      if (GET_KEYSTATE_WPARAM(wparam) & MK_RBUTTON) {
        return MouseButton::Right;
      }
      if (GET_KEYSTATE_WPARAM(wparam) & MK_MBUTTON) {
        return MouseButton::Middle;
      }
      return MouseButton::Left;
    default:
      return MouseButton::Left;
  }
}

void Window::DispatchMouse(UINT msg, WPARAM wparam, LPARAM lparam) {
  if (!root_ && !popup_) {
    return;
  }

  if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN ||
      msg == WM_MBUTTONDOWN || msg == WM_MOUSEWHEEL) {
    HideTooltip();
  }

  if (msg == WM_MOUSEMOVE) {
    EnsureMouseLeaveTracking();
  }

  MouseEvent ev;
  ev.button = ButtonFromMsg(msg, wparam);
  ev.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
  ev.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

  // Client coords from Win32 are physical pixels; Node tree is DIP.
  if (msg == WM_MOUSEWHEEL) {
    POINT pt = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
    ScreenToClient(hwnd_, &pt);
    ev.x = auralite::DipFromPx(static_cast<float>(pt.x), dpi_);
    ev.y = auralite::DipFromPx(static_cast<float>(pt.y), dpi_);
    ev.wheel_delta = GET_WHEEL_DELTA_WPARAM(wparam);
  } else {
    ev.x = auralite::DipFromPx(static_cast<float>(GET_X_LPARAM(lparam)), dpi_);
    ev.y = auralite::DipFromPx(static_cast<float>(GET_Y_LPARAM(lparam)), dpi_);
  }

  SyncPopupLayout();

  Node* popup_hit = popup_ ? popup_->HitTest(ev.x, ev.y) : nullptr;
  Node* root_hit = root_ ? root_->HitTest(ev.x, ev.y) : nullptr;

  if ((msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN ||
       msg == WM_MBUTTONDOWN) &&
      popup_ && !popup_hit) {
    if (!(popup_anchor_ && root_hit == popup_anchor_)) {
      ClearPopup();
      root_hit = root_ ? root_->HitTest(ev.x, ev.y) : nullptr;
    }
  }

  Node* hit = popup_hit ? popup_hit : root_hit;

  // PopupHost §4.3: hover/click a non-opener sibling while a child submenu
  // is open → dismiss child layers (before enter so Submenu can re-Push).
  if (popup_mode_ && hit &&
      (msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN ||
       msg == WM_MBUTTONDOWN)) {
    if (PopupHost* host = PopupHost::Current()) {
      host->OnPopupHit(this, hit);
    }
  }

  if (msg == WM_MOUSEMOVE && !mouse_capture_) {
    if (hovered_ != hit) {
      if (hovered_) {
        hovered_->OnMouseLeave(ev);
      }
      hovered_ = hit;
      if (hovered_) {
        hovered_->OnMouseEnter(ev);
      }
      RestartTooltipTimer();
    }
  }

  Node* target = mouse_capture_ ? mouse_capture_ : hit;
  if (!target) {
    if (msg == WM_MOUSEMOVE) {
      Invalidate();
    }
    return;
  }

  switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
      mouse_capture_ = target;
      SetCapture(hwnd_);
      if (target->focusable()) {
        SetFocusNode(target);
      }
      target->OnMouseDown(ev);
      break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
      target->OnMouseUp(ev);
      if (GetCapture() == hwnd_) {
        ReleaseCapture();
      }
      mouse_capture_ = nullptr;
      // Refresh hover after release (cursor may still be over a control).
      if (hovered_ != hit) {
        if (hovered_) {
          hovered_->OnMouseLeave(ev);
        }
        hovered_ = hit;
        if (hovered_) {
          hovered_->OnMouseEnter(ev);
        }
      }
      break;
    case WM_MOUSEMOVE:
      target->OnMouseMove(ev);
      break;
    case WM_MOUSEWHEEL: {
      // Prefer first scrollable ancestor from the hit node (ScrollView).
      Node* wheel_target = target;
      while (wheel_target && !wheel_target->WantsMouseWheel()) {
        wheel_target = wheel_target->parent();
      }
      if (wheel_target) {
        wheel_target->OnMouseWheel(ev);
      }
      break;
    }
    default:
      break;
  }

  if (clear_popup_pending_) {
    ClearPopup();
  }

  // PopupHost menu dismiss after click/Esc — must run before Invalidate so we
  // do not touch |this| if Flush destroys this Window.
  if (popup_mode_) {
    if (PopupHost* host = PopupHost::Current()) {
      if (host->has_pending_dismiss()) {
        host->FlushPendingDismiss();
        return;
      }
    }
  }

  Invalidate();
}

void Window::DispatchContextMenu(WPARAM /*wparam*/, LPARAM lparam) {
  if (!root_) {
    return;
  }

  POINT screen = {};
  float hit_x = 0.f;
  float hit_y = 0.f;
  if (lparam == static_cast<LPARAM>(-1)) {
    // Keyboard invocation (Shift+F10 / VK_APPS): use focused node or client
    // center. bounds_ / ClientRectF are DIP.
    if (focused_) {
      const RectF b = focused_->bounds();
      hit_x = b.x + b.w * 0.5f;
      hit_y = b.y + b.h * 0.5f;
    } else {
      const RectF c = ClientRectF();
      hit_x = c.w * 0.5f;
      hit_y = c.h * 0.5f;
    }
    POINT client_px{static_cast<LONG>(auralite::PxFromDip(hit_x, dpi_)),
                    static_cast<LONG>(auralite::PxFromDip(hit_y, dpi_))};
    screen = client_px;
    ClientToScreen(hwnd_, &screen);
  } else {
    screen.x = GET_X_LPARAM(lparam);
    screen.y = GET_Y_LPARAM(lparam);
    POINT client_px = screen;
    ScreenToClient(hwnd_, &client_px);
    hit_x = auralite::DipFromPx(static_cast<float>(client_px.x), dpi_);
    hit_y = auralite::DipFromPx(static_cast<float>(client_px.y), dpi_);
  }

  Node* hit = root_->HitTest(hit_x, hit_y);
  if (!hit && focused_) {
    hit = focused_;
  }
  if (hit) {
    hit->OnContextMenu(screen.x, screen.y);
  }
}

void Window::DispatchKey(UINT msg, WPARAM wparam) {
  // PopupHost: Esc closes the top layer only (DismissFrom(depth-1)).
  if (popup_mode_ && (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) &&
      wparam == VK_ESCAPE) {
    if (PopupHost* host = PopupHost::Current()) {
      const size_t d = host->depth();
      if (d >= 1) {
        host->RequestDismissFrom(d - 1);
      }
      if (host->FlushPendingDismiss()) {
        return;
      }
    }
  }

  if (dialog_mode_ && (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) &&
      wparam == VK_ESCAPE) {
    EndModal(IDCANCEL);
    return;
  }

  if (!focused_) {
    return;
  }
  KeyEvent ev;
  ev.vk = static_cast<UINT>(wparam);
  ev.down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
  ev.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
  ev.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
  ev.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
  focused_->OnKey(ev);
  Invalidate();
}

void Window::DispatchChar(WPARAM wparam) {
  if (!focused_) {
    return;
  }
  // Skip Tab / Return / Esc / Backspace (handled via WM_KEYDOWN).
  if (wparam == VK_TAB || wparam == VK_RETURN || wparam == VK_ESCAPE ||
      wparam == VK_BACK) {
    return;
  }
  // Swallow WM_CHAR duplicates after Imm GCS_RESULTSTR.
  if (ime_char_suppress_ > 0) {
    --ime_char_suppress_;
    return;
  }
  focused_->OnChar(static_cast<wchar_t>(wparam));
  Invalidate();
}

void Window::DispatchImeChar(WPARAM wparam) {
  if (!focused_ || !focused_->WantsIme()) {
    return;
  }
  // Swallow duplicates already committed via WM_IME_COMPOSITION / GCS_RESULTSTR.
  if (ime_char_suppress_ > 0) {
    --ime_char_suppress_;
    return;
  }
  focused_->OnChar(static_cast<wchar_t>(wparam));
  Invalidate();
}

void Window::RestartTooltipTimer() {
  HideTooltip();
  if (!hwnd_ || popup_mode_ || dialog_mode_) {
    return;
  }
  KillTimer(hwnd_, kTooltipTimerId);
  if (!ResolveTooltipText(hovered_)) {
    return;
  }
  SetTimer(hwnd_, kTooltipTimerId, kTooltipDelayMs, nullptr);
}

void Window::HideTooltip() {
  if (hwnd_) {
    KillTimer(hwnd_, kTooltipTimerId);
  }
  if (tooltip_window_ && tooltip_window_->hwnd_) {
    ShowWindow(tooltip_window_->hwnd_, SW_HIDE);
  }
  tooltip_shown_text_ = nullptr;
  tooltip_text_.clear();
}

bool Window::EnsureTooltipWindow() {
  if (tooltip_window_ && tooltip_window_->hwnd_) {
    return true;
  }

  tooltip_window_ = std::make_unique<Window>();
  Window* tip = tooltip_window_.get();

  (void)Theme::Active();

  HINSTANCE instance = GetModuleHandleW(nullptr);
  if (!EnsureWindowClass(instance)) {
    tooltip_window_.reset();
    return false;
  }

  tip->popup_mode_ = true;
  tip->quit_on_close_ = false;

  tip->dpi_ = QueryMonitorDpiNearCursor();
  const int pw = DipToOuterPx(1.f, tip->dpi_);
  const int ph = DipToOuterPx(1.f, tip->dpi_);

  tip->hwnd_ = CreateWindowExW(
      WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED |
          WS_EX_TRANSPARENT,
      kWindowClassName, L"", WS_POPUP | WS_CLIPCHILDREN, 0, 0, pw, ph, hwnd_,
      nullptr, instance, tip);
  if (!tip->hwnd_) {
    tooltip_window_.reset();
    return false;
  }

  tip->dpi_ = QueryHwndDpi(tip->hwnd_);
  tip->canvas_.SetDpi(tip->dpi_);

  if (!tip->canvas_.InitLayered(tip->hwnd_)) {
    DestroyWindow(tip->hwnd_);
    tip->hwnd_ = nullptr;
    tooltip_window_.reset();
    return false;
  }

  ImmAssociateContextEx(tip->hwnd_, NULL, 0);

  tip->theme_sink_ = [tip] { tip->Invalidate(); };
  Theme::AddInvalidateSink(&tip->theme_sink_);

  tip->layout_dirty_ = true;
  return true;
}

void Window::PlaceTooltipWindow(float dip_w, float dip_h) {
  if (!tooltip_window_ || !tooltip_window_->hwnd_) {
    return;
  }

  POINT pt = {};
  GetCursorPos(&pt);
  const int offset_y =
      static_cast<int>(auralite::PxFromDip(16.f, dpi_));
  int x = pt.x;
  int y = pt.y + offset_y;
  const int pw = DipToOuterPx(dip_w, dpi_);
  const int ph = DipToOuterPx(dip_h, dpi_);

  HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
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

  SetWindowPos(tooltip_window_->hwnd_, HWND_TOPMOST, x, y, pw, ph,
               SWP_NOACTIVATE);
}

void Window::ShowTooltipFor(const Node* hit) {
  const std::wstring* text = ResolveTooltipText(hit);
  if (!text || !hwnd_) {
    return;
  }
  // Own a copy — ResolveTooltipText returns Node interior storage that must
  // not outlive Win32 callbacks / tree mutations.
  tooltip_text_ = *text;
  if (!EnsureTooltipWindow()) {
    tooltip_text_.clear();
    return;
  }
  const ThemeTokens& t = Theme::Active();
  auto label = std::make_unique<Label>();
  label->text(tooltip_text_)
      .font_size(t.font_size_sm)
      .color(t.text)
      .hug_width()
      .hug_height();
  float tw =
      canvas_.MeasureTextWidth(tooltip_text_, t.font_size_sm, t.font_ui.c_str());
  const float pad_x = 8.f;
  const float pad_y = 6.f;
  tw = (std::min)(tw + pad_x * 2.f, 320.f);
  const float th = t.font_size_sm + pad_y * 2.f;
  auto root = std::make_unique<Node>();
  root->bg(t.surface);
  root->clip_children(true);
  root->fixed_width(tw).fixed_height(th);
  root->AddChild(std::move(label));
  tooltip_window_->SetRoot(std::move(root));
  PlaceTooltipWindow(tw, th);
  ShowWindow(tooltip_window_->hwnd(), SW_SHOWNOACTIVATE);
  tooltip_shown_text_ = &tooltip_text_;
}

}  // namespace auralite::ui
