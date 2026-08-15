#include <windows.h>
#include <objbase.h>

#include <memory>

#include "auralite/ui/application.h"
#include "auralite/ui/column.h"
#include "auralite/ui/node.h"
#include "auralite/ui/window.h"

namespace {

// Fixed preferred size; Column stretches width on the cross axis.
class ColorBlock : public auralite::ui::Node {
 public:
  ColorBlock(auralite::ColorF color, float pref_w, float pref_h)
      : color_(color), pref_w_(pref_w), pref_h_(pref_h) {}

  auralite::ui::SizeF Measure(float /*max_w*/, float /*max_h*/) override {
    return auralite::ui::SizeF{pref_w_, pref_h_};
  }

  void Paint(auralite::Canvas& canvas) override {
    canvas.FillRect(bounds_, color_);
  }

 private:
  auralite::ColorF color_;
  float pref_w_ = 0.f;
  float pref_h_ = 0.f;
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

  auto root = std::make_unique<auralite::ui::Column>();
  root->padding(24.f).spacing(12.f);
  root->AddChild(std::make_unique<ColorBlock>(
      auralite::ColorF::FromRgb(220, 70, 70), 200.f, 64.f));
  root->AddChild(std::make_unique<ColorBlock>(
      auralite::ColorF::FromRgb(50, 160, 90), 200.f, 64.f));
  root->AddChild(std::make_unique<ColorBlock>(
      auralite::ColorF::FromRgb(40, 110, 200), 200.f, 64.f));

  window.SetRoot(std::move(root));
  ShowWindow(window.hwnd(), show);
  UpdateWindow(window.hwnd());

  const int code = auralite::ui::Application::Run();
  CoUninitialize();
  return code;
}
