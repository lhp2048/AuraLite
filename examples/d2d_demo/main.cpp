#include <windows.h>
#include <objbase.h>
#include <string>
#include <vector>

#include "auralite/canvas.h"

namespace {

auralite::Canvas* g_canvas = nullptr;
auralite::Image* g_image = nullptr;

void EnableDpiAwareness() {
  using SetDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
  HMODULE user32 = GetModuleHandleW(L"user32.dll");
  if (user32) {
    auto fn = reinterpret_cast<SetDpiAwarenessContextFn>(
        GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (fn) {
      fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
      return;
    }
  }
  SetProcessDPIAware();
}

bool BuildCheckerImage() {
  if (!g_canvas || !g_image) {
    return false;
  }
  const UINT size = 64;
  std::vector<uint8_t> pixels(size * size * 4);
  for (UINT y = 0; y < size; ++y) {
    for (UINT x = 0; x < size; ++x) {
      const bool on = ((x / 8) + (y / 8)) % 2 == 0;
      const UINT i = (y * size + x) * 4;
      // Premultiplied BGRA
      pixels[i + 0] = on ? 40 : 200;   // B
      pixels[i + 1] = on ? 110 : 200;  // G
      pixels[i + 2] = on ? 200 : 220;  // R
      pixels[i + 3] = 255;             // A
    }
  }
  return g_image->CreateFromBgra(*g_canvas, size, size, pixels.data(),
                                 size * 4);
}

void PaintDemo(HWND hwnd) {
  if (!g_canvas) {
    return;
  }
  RECT rc = {};
  GetClientRect(hwnd, &rc);
  const float w = static_cast<float>(rc.right);
  const float h = static_cast<float>(rc.bottom);

  if (!g_canvas->BeginDraw()) {
    return;
  }

  g_canvas->Clear(auralite::ColorF::FromRgb(245, 248, 252));

  g_canvas->FillRoundedRect({40.f, 40.f, 280.f, 120.f}, 16.f, 16.f,
                            auralite::ColorF::FromRgb(40, 110, 200));
  g_canvas->DrawText(L"AuraLite Direct2D", {56.f, 70.f, 250.f, 40.f},
                     auralite::ColorF::FromRgb(255, 255, 255), 22.f);

  g_canvas->FillRect({40.f, 190.f, 180.f, 80.f},
                     auralite::ColorF::FromRgb(220, 90, 70));
  g_canvas->DrawRect({40.f, 190.f, 180.f, 80.f},
                     auralite::ColorF::FromRgb(120, 40, 30), 2.f);

  if (g_image && !g_image->empty()) {
    g_canvas->DrawImage(*g_image, {250.f, 190.f, 96.f, 96.f});
  }

  g_canvas->DrawText(
      L"阶段一：矩形 / 圆角 / DirectWrite / 位图",
      {40.f, 320.f, w - 80.f, 40.f}, auralite::ColorF::FromRgb(30, 40, 55),
      18.f);

  g_canvas->DrawText(
      L"CMake 已编入 Base + UI + auralite_d2d",
      {40.f, h - 56.f, w - 80.f, 40.f}, auralite::ColorF::FromRgb(90, 100, 120),
      14.f);

  if (!g_canvas->EndDraw()) {
    // Device lost: recreate target and CPU-backed demo bitmap.
    g_canvas->Init(hwnd);
    BuildCheckerImage();
  }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_CREATE: {
      g_canvas = new auralite::Canvas();
      g_image = new auralite::Image();
      if (!g_canvas->Init(hwnd) || !BuildCheckerImage()) {
        MessageBoxW(hwnd, L"Direct2D 初始化失败", L"d2d_demo", MB_ICONERROR);
        return -1;
      }
      return 0;
    }
    case WM_SIZE: {
      if (g_canvas && wparam != SIZE_MINIMIZED) {
        g_canvas->Resize(LOWORD(lparam), HIWORD(lparam));
        InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }
    case WM_DISPLAYCHANGE:
    case WM_PAINT: {
      PAINTSTRUCT ps = {};
      BeginPaint(hwnd, &ps);
      PaintDemo(hwnd);
      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_DESTROY:
      delete g_image;
      g_image = nullptr;
      delete g_canvas;
      g_canvas = nullptr;
      PostQuitMessage(0);
      return 0;
    default:
      break;
  }
  return DefWindowProcW(hwnd, msg, wparam, lparam);
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int show) {
  EnableDpiAwareness();
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  const wchar_t* kClass = L"AuraLiteD2DDemo";
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = instance;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = nullptr;
  wc.lpszClassName = kClass;
  if (!RegisterClassExW(&wc)) {
    CoUninitialize();
    return 1;
  }

  HWND hwnd = CreateWindowExW(
      0, kClass, L"AuraLite D2D Demo", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 720, 480, nullptr, nullptr, instance,
      nullptr);
  if (!hwnd) {
    CoUninitialize();
    return 1;
  }

  ShowWindow(hwnd, show);
  UpdateWindow(hwnd);

  MSG msg = {};
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  CoUninitialize();
  return static_cast<int>(msg.wParam);
}
