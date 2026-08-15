#pragma once

namespace auralite::ui {

class Application {
 public:
  // Per-monitor V2 when available; falls back to SetProcessDPIAware.
  static void EnableDpiAwareness();

  // Pump the Win32 message loop until WM_QUIT. Returns exit code.
  static int Run();
};

}  // namespace auralite::ui
