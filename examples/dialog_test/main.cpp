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
    Window w;
    Expect(w.RunModal() == IDABORT, "RunModal no hwnd");
    w.EndModal(IDOK);
    Expect(w.modal_result() == IDOK, "EndModal without Run stores result");
  }

  {
    Window w;
    Expect(w.CreateDialogWindow(nullptr, 200, 120), "CreateDialogWindow");
    HWND hwnd = w.hwnd();
    Expect(hwnd != nullptr, "hwnd");
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    Expect((style & WS_CAPTION) == 0, "no caption");
    Expect((style & WS_POPUP) != 0, "WS_POPUP");
    DestroyWindow(hwnd);
  }

  base::AtExitManager at_exit;
  MessageLoopForUI loop;
  Window dlg;
  Window owner;
  Expect(owner.Create(L"owner", 320, 240), "owner Create");
  EnableWindow(owner.hwnd(), TRUE);
  Expect(dlg.CreateDialogWindow(owner.hwnd(), 200, 120), "dlg create");
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
