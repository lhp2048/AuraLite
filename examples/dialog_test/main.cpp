#include <cstdio>
#include <windows.h>

#include "auralite/async/task_lambda.h"
#include "auralite/ui/window.h"
#include "base/at_exit.h"
#include "message_framework/message_loop.h"

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

}  // namespace

int main() {
  using auralite::ui::Window;

  {
    Window::WindowOptions o;
    o.caption = true;
    o.corner_radius = -3.f;
    o.border_width = -1.f;
    o.Normalize();
    Expect(o.corner_radius == 0.f, "Normalize caption radius");
    Expect(o.border_width == 0.f, "Normalize caption border");
    Window::WindowOptions d = Window::WindowOptions::Dialog(nullptr, -4.f);
    Expect(!d.caption, "Dialog no caption");
    Expect(d.corner_radius == 0.f, "Dialog negative radius");
    Expect(d.border_width == 1.f, "Dialog border");
  }

  {
    Window w;
    Expect(w.RunModal() == IDABORT, "RunModal no hwnd");
    w.EndModal(IDOK);
    Expect(w.modal_result() == IDOK, "EndModal without Run stores result");
  }

  {
    Window app;
    Expect(app.Create(L"app", 200, 120), "Create captioned");
    HWND hwnd = app.hwnd();
    Expect(hwnd != nullptr, "app hwnd");
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    Expect((style & WS_CAPTION) != 0, "app has caption");
    DestroyWindow(hwnd);
  }

  {
    Window app;
    Window::WindowOptions o;
    o.caption = true;
    o.corner_radius = 8.f;
    o.border_width = 1.f;
    Expect(app.Create(L"app", 200, 120, o), "Create captioned with radius");
    HWND hwnd = app.hwnd();
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    Expect((style & WS_CAPTION) != 0, "captioned still has caption");
    Expect(app.options().caption, "options caption true");
    Expect(app.options().corner_radius == 0.f, "options stores normalized radius");
    Expect(app.options().border_width == 0.f, "options stores normalized border");
    Expect(app.RunModal() == IDABORT, "captioned RunModal abort");
    HRGN probe = CreateRectRgn(0, 0, 0, 0);
    const int rgn_type = GetWindowRgn(hwnd, probe);
    Expect(rgn_type == ERROR, "captioned window has no library rgn");
    DeleteObject(probe);
    DestroyWindow(hwnd);
  }

  {
    Window w;
    Window::WindowOptions o;
    o.caption = false;
    o.quit_on_close = false;
    o.topmost = false;
    o.corner_radius = 8.f;
    o.border_width = 1.f;
    Expect(w.Create(L"modeless", 200, 120, o), "Create modeless");
    HWND hwnd = w.hwnd();
    Expect(hwnd != nullptr, "modeless hwnd");
    Expect(w.is_dialog(), "modeless custom chrome");
    Expect(!w.options().caption, "modeless options no caption");
    Expect(w.options().corner_radius == 8.f, "modeless keeps radius");
    Expect(w.options().quit_on_close == false, "modeless quit_on_close");
    SendMessageW(hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
    Expect(IsWindow(hwnd) && w.hwnd() == hwnd, "modeless Esc does not destroy");
    w.Close();
    Expect(!IsWindow(hwnd), "Close destroys modeless");
    Expect(w.hwnd() == nullptr, "Close clears hwnd");
  }

  {
    Window dlg;
    Expect(dlg.Create(L"dlg", 200, 120, Window::WindowOptions::Dialog()),
           "Create with Dialog options");
    HWND hwnd = dlg.hwnd();
    Expect(hwnd != nullptr, "dlg hwnd");
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    Expect((style & WS_CAPTION) == 0, "Dialog options no caption");
    Expect((style & WS_POPUP) != 0, "Dialog options WS_POPUP");
    Expect(dlg.is_dialog(), "Dialog options is_dialog");
    HRGN probe = CreateRectRgn(0, 0, 0, 0);
    const int rgn_type = GetWindowRgn(hwnd, probe);
    Expect(rgn_type == COMPLEXREGION || rgn_type == SIMPLEREGION,
           "Dialog options rounded rgn");
    DeleteObject(probe);
    DestroyWindow(hwnd);
  }

  {
    Window dlg;
    Expect(dlg.Create(L"square", 200, 120,
                      Window::WindowOptions::Dialog(nullptr, 0.f)),
           "Create square Dialog");
    HWND hwnd = dlg.hwnd();
    Expect(dlg.is_dialog(), "square is_dialog");
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    Expect((style & WS_CAPTION) == 0, "square no caption");
    HRGN probe = CreateRectRgn(0, 0, 0, 0);
    const int rgn_type = GetWindowRgn(hwnd, probe);
    Expect(rgn_type == ERROR, "square Dialog has no rgn");
    DeleteObject(probe);
    DestroyWindow(hwnd);
  }

  base::AtExitManager at_exit;
  MessageLoopForUI loop;
  Window dlg;
  Window owner;
  Expect(owner.Create(L"owner", 320, 240), "owner Create");
  EnableWindow(owner.hwnd(), TRUE);
  Expect(dlg.Create(L"dlg", 200, 120, Window::WindowOptions::Dialog(owner.hwnd())),
         "dlg create");
  auralite::async::PostFn([&] {
    Expect(!IsWindowEnabled(owner.hwnd()), "owner disabled in modal");
    dlg.EndModal(IDOK);
  });
  Expect(dlg.RunModal() == IDOK, "RunModal returns IDOK");
  Expect(IsWindowEnabled(owner.hwnd()), "owner re-enabled");

  if (g_failures) {
    std::printf("%d failed\n", g_failures);
    return 1;
  }
  std::printf("all passed\n");
  return 0;
}
