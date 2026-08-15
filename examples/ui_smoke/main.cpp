#include <windows.h>
#include <objbase.h>

#include <memory>
#include <vector>

#include "auralite/ui/application.h"
#include "auralite/ui/button.h"
#include "auralite/ui/column.h"
#include "auralite/ui/image_button.h"
#include "auralite/ui/image_view.h"
#include "auralite/ui/label.h"
#include "auralite/ui/window.h"

namespace {

std::vector<uint8_t> MakeCheckerBgra(UINT size, UINT cell) {
  std::vector<uint8_t> pixels(size * size * 4);
  for (UINT y = 0; y < size; ++y) {
    for (UINT x = 0; x < size; ++x) {
      const bool on = ((x / cell) + (y / cell)) % 2 == 0;
      const UINT i = (y * size + x) * 4;
      pixels[i + 0] = on ? 40 : 200;
      pixels[i + 1] = on ? 110 : 200;
      pixels[i + 2] = on ? 200 : 220;
      pixels[i + 3] = 255;
    }
  }
  return pixels;
}

std::vector<uint8_t> MakeSolidBgra(UINT size, uint8_t b, uint8_t g, uint8_t r) {
  std::vector<uint8_t> pixels(size * size * 4);
  for (UINT i = 0; i < size * size; ++i) {
    pixels[i * 4 + 0] = b;
    pixels[i * 4 + 1] = g;
    pixels[i * 4 + 2] = r;
    pixels[i * 4 + 3] = 255;
  }
  return pixels;
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int show) {
  auralite::ui::Application::EnableDpiAwareness();
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  auralite::ui::Window window;
  if (!window.Create(L"AuraLite UI Smoke", 720, 560)) {
    MessageBoxW(nullptr, L"Window / Canvas init failed", L"ui_smoke",
                MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  auto root = std::make_unique<auralite::ui::Column>();
  root->padding(24.f).spacing(10.f);

  {
    auto title = std::make_unique<auralite::ui::Label>();
    title->text(L"AuraLite Phase 2 — Label / Button / Image")
        .font_size(20.f)
        .align(auralite::ui::TextAlign::Left);
    root->AddChild(std::move(title));
  }

  auto status = std::make_unique<auralite::ui::Label>();
  auralite::ui::Label* status_ptr = status.get();
  status->text(L"Click the button…")
      .font_size(15.f)
      .color(auralite::ColorF::FromRgb(90, 100, 120))
      .align(auralite::ui::TextAlign::Center);
  root->AddChild(std::move(status));

  {
    auto left = std::make_unique<auralite::ui::Label>();
    left->text(L"align: left")
        .font_size(14.f)
        .align(auralite::ui::TextAlign::Left)
        .preferred_height(24.f);
    root->AddChild(std::move(left));
  }
  {
    auto center = std::make_unique<auralite::ui::Label>();
    center->text(L"align: center")
        .font_size(14.f)
        .align(auralite::ui::TextAlign::Center)
        .preferred_height(24.f);
    root->AddChild(std::move(center));
  }
  {
    auto right = std::make_unique<auralite::ui::Label>();
    right->text(L"align: right")
        .font_size(14.f)
        .align(auralite::ui::TextAlign::Right)
        .preferred_height(24.f);
    root->AddChild(std::move(right));
  }

  {
    auto click_btn = std::make_unique<auralite::ui::Button>();
    click_btn->text(L"Click me")
        .preferred_size(160.f, 40.f)
        .on_click([status_ptr, &window]() {
          if (status_ptr) {
            status_ptr->text(L"Button clicked!");
          }
          window.Invalidate();
        });
    root->AddChild(std::move(click_btn));
  }

  {
    auto msg_btn = std::make_unique<auralite::ui::Button>();
    msg_btn->text(L"MessageBox")
        .preferred_size(160.f, 40.f)
        .on_click([]() {
          MessageBoxW(nullptr, L"Hello from auralite::ui::Button", L"ui_smoke",
                      MB_OK | MB_ICONINFORMATION);
        });
    root->AddChild(std::move(msg_btn));
  }

  {
    const auto checker = MakeCheckerBgra(64, 8);
    auto image = std::make_unique<auralite::ui::ImageView>();
    image->SetPixels(64, 64, checker.data(), 64 * 4).preferred_size(96.f, 96.f);
    root->AddChild(std::move(image));
  }

  {
    const auto solid = MakeSolidBgra(32, 70, 160, 50);
    auto img_btn = std::make_unique<auralite::ui::ImageButton>();
    img_btn->SetPixels(32, 32, solid.data(), 32 * 4)
        .preferred_size(56.f, 56.f)
        .on_click([status_ptr, &window]() {
          if (status_ptr) {
            status_ptr->text(L"ImageButton clicked!");
          }
          window.Invalidate();
        });
    root->AddChild(std::move(img_btn));
  }

  window.SetRoot(std::move(root));
  ShowWindow(window.hwnd(), show);
  UpdateWindow(window.hwnd());

  const int code = auralite::ui::Application::Run();
  CoUninitialize();
  return code;
}
