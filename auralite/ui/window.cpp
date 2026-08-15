#include "auralite/ui/window.h"

#include <windowsx.h>

namespace auralite::ui {
namespace {

constexpr wchar_t kWindowClassName[] = L"AuraLite.UI.Window";

}  // namespace

Window::Window() = default;

Window::~Window() {
  if (hwnd_) {
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
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

  layout_dirty_ = true;
  return true;
}

void Window::SetRoot(std::unique_ptr<Node> root) {
  root_ = std::move(root);
  mouse_capture_ = nullptr;
  layout_dirty_ = true;
  Invalidate();
}

void Window::Invalidate() {
  if (hwnd_) {
    InvalidateRect(hwnd_, nullptr, FALSE);
  }
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
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
      DispatchMouse(msg, wparam, lparam);
      return 0;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
      DispatchKey(msg, wparam);
      return 0;

    case WM_DESTROY:
      canvas_.Shutdown();
      hwnd_ = nullptr;
      mouse_capture_ = nullptr;
      // Quit the app when the last UI window closes (Task 1 single-window).
      PostQuitMessage(0);
      return 0;

    default:
      break;
  }
  return DefWindowProcW(hwnd_, msg, wparam, lparam);
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

  if (!canvas_.is_valid() && !canvas_.Init(hwnd_)) {
    return;
  }

  if (!canvas_.BeginDraw()) {
    return;
  }

  canvas_.Clear(ColorF::FromRgb(245, 248, 252));

  if (root_) {
    const RectF client = ClientRectF();
    if (layout_dirty_) {
      root_->Layout(client);
      layout_dirty_ = false;
    }
    root_->Paint(canvas_);
  }

  if (!canvas_.EndDraw()) {
    canvas_.Init(hwnd_);
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
  if (!root_) {
    return;
  }

  MouseEvent ev;
  ev.button = ButtonFromMsg(msg, wparam);

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

  Node* target = mouse_capture_;
  if (!target) {
    target = root_->HitTest(ev.x, ev.y);
  }
  if (!target) {
    return;
  }

  switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
      mouse_capture_ = target;
      SetCapture(hwnd_);
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
      break;
    case WM_MOUSEMOVE:
      target->OnMouseMove(ev);
      break;
    case WM_MOUSEWHEEL:
      target->OnMouseWheel(ev);
      break;
    default:
      break;
  }
}

void Window::DispatchKey(UINT msg, WPARAM wparam) {
  if (!root_) {
    return;
  }
  KeyEvent ev;
  ev.vk = static_cast<UINT>(wparam);
  ev.down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
  ev.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
  ev.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
  ev.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
  // Focus routing lands in a later task; broadcast to root for now.
  root_->OnKey(ev);
}

}  // namespace auralite::ui
