#include "auralite/ui/application.h"

#include "auralite/async/awaiters.h"
#include "base/at_exit.h"
#include "message_framework/message_loop.h"

#include <windows.h>

namespace auralite::ui {

void Application::EnableDpiAwareness() {
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

int Application::Run() {
  // MessageLoop TLS is a LazyInstance; RegisterCallback requires a manager.
  // Skip creating a second manager when the host (e.g. Family Shell) already
  // owns one for the process.
  //
  // Lifetime: if we create the manager here it must outlive WinMain locals
  // (Windows). Static storage is intentional for the "Run owns process" path.
  struct OptionalAtExit {
    base::AtExitManager* owned = nullptr;
    OptionalAtExit() {
      if (!base::AtExitManager::IsInitialized()) {
        owned = new base::AtExitManager();
      }
    }
    ~OptionalAtExit() { delete owned; }
  };
  static OptionalAtExit exit_holder;

  MessageLoopForUI loop;
  auralite::async::EnsureUiProxy();
  // Default: process all Windows messages (null dispatcher uses pump default).
  loop.Run(nullptr);
  return 0;
}

}  // namespace auralite::ui
