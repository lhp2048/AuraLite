#include "auralite/ui/window.h"

#include "auralite/async/task_lambda.h"
#include "message_framework/message_loop.h"

#include <imm.h>
#include <windowsx.h>

#include <string>
#include <vector>

namespace auralite::ui {
namespace {

constexpr wchar_t kWindowClassName[] = L"AuraLite.UI.Window";

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
  if (hwnd_) {
    if (anim_clients_ > 0) {
      KillTimer(hwnd_, kAnimTimerId);
      anim_clients_ = 0;
    }
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
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

  hwnd_ = CreateWindowExW(
      0, kWindowClassName, title ? title : L"AuraLite", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, w, h, nullptr, nullptr, instance, this);
  if (!hwnd_) {
    return false;
  }

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
  // Coalesce multiple Invalidate calls in the same turn into one PostTask.
  MessageLoop* loop = MessageLoop::current();
  if (!loop) {
    InvalidateRect(hwnd_, nullptr, FALSE);
    return;
  }
  if (invalidate_posted_) {
    return;
  }
  invalidate_posted_ = true;
  auto alive = alive_;
  loop->PostTask(new auralite::async::LambdaTask([this, alive] {
    invalidate_posted_ = false;
    if (!alive || !alive->load(std::memory_order_acquire) || !hwnd_) {
      return;
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
  }));
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
  form.ptCurrentPos.x = static_cast<LONG>(b.x + 10.f);
  form.ptCurrentPos.y = static_cast<LONG>(b.y + b.h);
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
      break;

    case WM_DESTROY:
      if (anim_clients_ > 0) {
        KillTimer(hwnd_, kAnimTimerId);
        anim_clients_ = 0;
      }
      canvas_.Shutdown();
      hwnd_ = nullptr;
      mouse_capture_ = nullptr;
      hovered_ = nullptr;
      focused_ = nullptr;
      tracking_mouse_leave_ = false;
      // Quit the app when the last UI window closes (Task 1 single-window).
      PostQuitMessage(0);
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
  return RectF{0.f, 0.f, static_cast<float>(rc.right),
               static_cast<float>(rc.bottom)};
}

void Window::OnSize(UINT width, UINT height) {
  canvas_.Resize(width, height);
  layout_dirty_ = true;
  Invalidate();
}

void Window::OnPaint() {
  if (!hwnd_) {
    return;
  }

  const bool need_init = !canvas_.is_valid();
  if (need_init && !canvas_.Init(hwnd_)) {
    return;
  }
  if (need_init) {
    NotifyDeviceLost();
  }

  if (!canvas_.BeginDraw()) {
    return;
  }

  canvas_.Clear(Theme::Active().window_bg);

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
    if (canvas_.Init(hwnd_)) {
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

  if (msg == WM_MOUSEMOVE) {
    EnsureMouseLeaveTracking();
  }

  MouseEvent ev;
  ev.button = ButtonFromMsg(msg, wparam);
  ev.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
  ev.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

  // Same pixel space as Canvas (kUiDpi) and Node::bounds_ / HitTest.
  if (msg == WM_MOUSEWHEEL) {
    POINT pt = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
    ScreenToClient(hwnd_, &pt);
    ev.x = static_cast<float>(pt.x);
    ev.y = static_cast<float>(pt.y);
    ev.wheel_delta = GET_WHEEL_DELTA_WPARAM(wparam);
  } else {
    ev.x = static_cast<float>(GET_X_LPARAM(lparam));
    ev.y = static_cast<float>(GET_Y_LPARAM(lparam));
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

  if (msg == WM_MOUSEMOVE && !mouse_capture_) {
    if (hovered_ != hit) {
      if (hovered_) {
        hovered_->OnMouseLeave(ev);
      }
      hovered_ = hit;
      if (hovered_) {
        hovered_->OnMouseEnter(ev);
      }
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

  Invalidate();
}

void Window::DispatchContextMenu(WPARAM /*wparam*/, LPARAM lparam) {
  if (!root_) {
    return;
  }

  POINT screen = {};
  POINT client = {};
  if (lparam == static_cast<LPARAM>(-1)) {
    // Keyboard invocation (Shift+F10 / VK_APPS): use focused node or client
    // center.
    if (focused_) {
      const RectF b = focused_->bounds();
      client.x = static_cast<LONG>(b.x + b.w * 0.5f);
      client.y = static_cast<LONG>(b.y + b.h * 0.5f);
    } else {
      const RectF c = ClientRectF();
      client.x = static_cast<LONG>(c.w * 0.5f);
      client.y = static_cast<LONG>(c.h * 0.5f);
    }
    screen = client;
    ClientToScreen(hwnd_, &screen);
  } else {
    screen.x = GET_X_LPARAM(lparam);
    screen.y = GET_Y_LPARAM(lparam);
    client = screen;
    ScreenToClient(hwnd_, &client);
  }

  Node* hit = root_->HitTest(static_cast<float>(client.x),
                             static_cast<float>(client.y));
  if (!hit && focused_) {
    hit = focused_;
  }
  if (hit) {
    hit->OnContextMenu(screen.x, screen.y);
  }
}

void Window::DispatchKey(UINT msg, WPARAM wparam) {
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

}  // namespace auralite::ui
