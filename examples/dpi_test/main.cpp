#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "mx/canvas.h"
#include "mx/ui/application.h"
#include "mx/ui/column.h"
#include "mx/ui/window.h"

namespace {

int g_failures = 0;

void Expect(bool cond, const char* name) {
  if (!cond) {
    std::printf("FAIL %s\n", name);
    ++g_failures;
  } else {
    std::printf("ok   %s\n", name);
  }
}

void ExpectNear(float got, float want, const char* name, float eps = 0.01f) {
  if (std::fabs(got - want) > eps) {
    std::printf("FAIL %s: got=%.4f want=%.4f\n", name, got, want);
    ++g_failures;
  } else {
    std::printf("ok   %s\n", name);
  }
}

void TestDcTargetMapsDip() {
  mx::ui::Application::EnableDpiAwareness();

  WNDCLASSW wc = {};
  wc.lpfnWndProc = DefWindowProcW;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wc.lpszClassName = L"MxUI.DpiProbe";
  RegisterClassW(&wc);
  HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"", WS_POPUP, 0, 0, 240,
                              240, nullptr, nullptr, wc.hInstance, nullptr);
  Expect(hwnd != nullptr, "probe hwnd");
  if (!hwnd) {
    return;
  }

  mx::Canvas canvas;
  canvas.SetDpi(192.f);
  Expect(canvas.Init(hwnd), "canvas init");
  Expect(canvas.BeginDraw(), "begin draw");
  if (!canvas.render_target()) {
    Expect(false, "render target");
    DestroyWindow(hwnd);
    return;
  }

  float dx = 0.f;
  float dy = 0.f;
  canvas.render_target()->GetDpi(&dx, &dy);
  const D2D1_SIZE_F dip = canvas.render_target()->GetSize();
  const D2D1_SIZE_U px = canvas.render_target()->GetPixelSize();
  std::printf("GetDpi=%.1f,%.1f GetSize=%.1fx%.1f GetPixelSize=%ux%u\n", dx, dy,
              dip.width, dip.height, px.width, px.height);
  // 240px DIB at 192 DPI must be 120 DIP so layout 96 DIP fills 192 px.
  ExpectNear(dx, 192.f, "dc rt dpi x after BindDC");
  ExpectNear(dip.width, 120.f, "dc rt dip width", 1.f);

  canvas.Clear(mx::ColorF(1.f, 1.f, 1.f, 1.f));
  canvas.FillRect(mx::RectF{0.f, 0.f, 96.f, 96.f},
                  mx::ColorF(1.f, 0.f, 0.f, 1.f));
  Expect(canvas.EndDraw(nullptr), "end draw");

  uint8_t b = 0, g = 0, r = 0, a = 0;
  Expect(canvas.PeekDibBgra(8, 8, &b, &g, &r, &a), "peek origin");
  std::printf("dib(8,8) BGRA=%u,%u,%u,%u\n", b, g, r, a);
  Expect(r > 200 && g < 40, "96dip origin is red");
  Expect(canvas.PeekDibBgra(150, 150, &b, &g, &r, &a), "peek scaled");
  std::printf("dib(150,150) BGRA=%u,%u,%u,%u\n", b, g, r, a);
  Expect(r > 200 && g < 40, "96dip maps to 192px");
  Expect(canvas.PeekDibBgra(220, 220, &b, &g, &r, &a), "peek outside");
  std::printf("dib(220,220) BGRA=%u,%u,%u,%u\n", b, g, r, a);
  Expect(g > 200 && r > 200, "outside 192px stays white");

  DestroyWindow(hwnd);
}

void TestWindowFillsClient() {
  mx::ui::Window window;
  Expect(window.Create(L"dpi-fill", 240, 240), "window create");
  auto root = std::make_unique<mx::ui::Column>();
  root->fill_width().fill_height();
  root->bg(mx::ColorF(1.f, 0.f, 0.f, 1.f));
  window.SetRoot(std::move(root));
  ShowWindow(window.hwnd(), SW_SHOWNOACTIVATE);
  UpdateWindow(window.hwnd());

  RECT rc = {};
  GetClientRect(window.hwnd(), &rc);
  std::printf("window dpi=%.1f client=%ldx%ld\n", window.dpi(),
              static_cast<long>(rc.right), static_cast<long>(rc.bottom));
  if (rc.right < 8 || rc.bottom < 8) {
    Expect(false, "client size");
    return;
  }

  uint8_t b = 0, g = 0, r = 0, a = 0;
  Expect(window.PeekDibBgra(2, 2, &b, &g, &r, &a), "peek window origin");
  std::printf("dib tl BGRA=%u,%u,%u,%u\n", b, g, r, a);
  Expect(r > 200 && g < 40, "client top-left red");
  Expect(window.PeekDibBgra(rc.right - 3, rc.bottom - 3, &b, &g, &r, &a),
         "peek window corner");
  std::printf("dib br BGRA=%u,%u,%u,%u\n", b, g, r, a);
  Expect(r > 200 && g < 40, "client bottom-right red");
}

}  // namespace

int main() {
  using mx::DipFromPx;
  using mx::PxFromDip;
  const float dpis[] = {96.f, 120.f, 144.f, 192.f};
  for (float dpi : dpis) {
    ExpectNear(PxFromDip(96.f, dpi), dpi, "96dip -> dpi px");
    ExpectNear(DipFromPx(dpi, dpi), 96.f, "dpi px -> 96dip");
    ExpectNear(DipFromPx(PxFromDip(800.f, dpi), dpi), 800.f, "roundtrip 800");
  }
  ExpectNear(PxFromDip(96.f, 0.f), 96.f, "dpi 0 treated as 96");
  ExpectNear(PxFromDip(96.f, -1.f), 96.f, "dpi neg treated as 96");
  Expect(std::ceil(PxFromDip(800.f, 144.f)) == 1200.0, "ceil 800dip @144 = 1200");
  TestDcTargetMapsDip();
  TestWindowFillsClient();
  if (g_failures) {
    std::printf("%d failed\n", g_failures);
    return 1;
  }
  std::printf("all passed\n");
  return 0;
}
