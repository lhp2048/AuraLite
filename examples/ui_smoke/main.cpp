#include <windows.h>
#include <objbase.h>

#include <memory>

#include "auralite/ui/application.h"
#include "auralite/ui/node.h"
#include "auralite/ui/window.h"

namespace {

class SmokeRoot : public auralite::ui::Node {
 public:
  void Paint(auralite::Canvas& canvas) override {
    using auralite::ColorF;
    using auralite::ui::RectF;

    canvas.FillRect(bounds_, ColorF::FromRgb(245, 248, 252));

    const RectF card{40.f, 40.f, bounds_.w - 80.f, 120.f};
    canvas.FillRoundedRect(card, 12.f, 12.f, ColorF::FromRgb(40, 110, 200));
    canvas.DrawText(L"ui smoke",
                    {card.x + 20.f, card.y + 36.f, card.w - 40.f, 48.f},
                    ColorF::FromRgb(255, 255, 255), 28.f);

    canvas.DrawText(L"AuraLite::UINext frame loop",
                    {40.f, 190.f, bounds_.w - 80.f, 32.f},
                    ColorF::FromRgb(30, 40, 55), 16.f);
  }
};

}  // namespace

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int show) {
  auralite::ui::Application::EnableDpiAwareness();
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  auralite::ui::Window window;
  if (!window.Create(L"AuraLite UI Smoke", 720, 480)) {
    MessageBoxW(nullptr, L"Window / Canvas init failed", L"ui_smoke",
                MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  window.SetRoot(std::make_unique<SmokeRoot>());
  ShowWindow(window.hwnd(), show);
  UpdateWindow(window.hwnd());

  const int code = auralite::ui::Application::Run();
  CoUninitialize();
  return code;
}
