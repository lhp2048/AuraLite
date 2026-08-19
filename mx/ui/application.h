#pragma once

namespace mx::ui {

class Application {
 public:
  // Per-monitor V2 when available; falls back to SetProcessDPIAware.
  static void EnableDpiAwareness();

  // Pump Win32 via MessageLoopForUI until WM_QUIT. Creates a live UI
  // MessageLoop on this thread (required for PostTask / current()).
  static int Run();
};

}  // namespace mx::ui
