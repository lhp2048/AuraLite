#include <windows.h>
#include <objbase.h>

#include <memory>
#include <string>
#include <vector>
#include <exception>

#include "auralite/ui/application.h"
#include "auralite/ui/button.h"
#include "auralite/ui/checkbox.h"
#include "auralite/ui/column.h"
#include "auralite/ui/context_menu.h"
#include "auralite/ui/dsl.h"
#include "auralite/ui/factory.h"
#include "auralite/ui/image_button.h"
#include "auralite/ui/image_view.h"
#include "auralite/ui/label.h"
#include "auralite/ui/list_view.h"
#include "auralite/ui/radio.h"
#include "auralite/ui/row.h"
#include "auralite/ui/scroll_view.h"
#include "auralite/ui/split_view.h"
#include "auralite/ui/switch_control.h"
#include "auralite/ui/text_field.h"
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
  if (!window.Create(L"AuraLite UI Smoke", 720, 980)) {
    MessageBoxW(nullptr, L"Window / Canvas init failed", L"ui_smoke",
                MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  auralite::ui::ContextMenu context_menu;
  context_menu.AddItem(1, L"Refresh status")
      .AddSeparator()
      .AddItem(2, L"About AuraLite")
      .AddItem(3, L"Reset split ratio");

  auto root = std::make_unique<auralite::ui::Column>();
  auralite::ui::Column* root_ptr = root.get();
  root->padding(24.f).spacing(10.f);

  {
    auto title = std::make_unique<auralite::ui::Label>();
    title->text(L"AuraLite Phase 2 — ViewFactory / YAML / DSL")
        .font_size(20.f)
        .align(auralite::ui::TextAlign::Left);
    root->AddChild(std::move(title));
  }

  auto status = std::make_unique<auralite::ui::Label>();
  auralite::ui::Label* status_ptr = status.get();
  status->text(L"Drag the split · right-click for ContextMenu")
      .font_size(14.f)
      .color(auralite::ColorF::FromRgb(90, 100, 120))
      .align(auralite::ui::TextAlign::Left);
  root->AddChild(std::move(status));

  // Mini YAML vs DSL tree smoke (same shape + key props: Column → Label + Button).
  {
    using namespace auralite::ui::dsl;
    auto dsl_tree =
        Column()
            .padding(8.f)
            .spacing(6.f)
            .child(Label().text(L"YAML+DSL mini").font_size(14.f))
            .child(Button().text(L"Mini Button").preferred_size(140.f, 36.f))
            .Build();

    constexpr const char* kMiniYaml = R"(
Column:
  padding: 8
  spacing: 6
  children:
    - Label: { text: "YAML+DSL mini", font_size: 14 }
    - Button: { text: "Mini Button", width: 140, height: 36, on_click: yaml_click }
)";

    auralite::ui::HandlerMap handlers;
    handlers["yaml_click"] = [status_ptr, &window]() {
      if (status_ptr) {
        status_ptr->text(L"YAML Button clicked (HandlerMap)");
      }
      window.Invalidate();
    };

    auralite::ui::ViewFactory factory;
    std::unique_ptr<auralite::ui::Node> yaml_tree;
    try {
      yaml_tree = factory.CreateFromYamlString(kMiniYaml, handlers);
    } catch (const std::exception& ex) {
      MessageBoxA(nullptr, ex.what(), "YAML load failed", MB_ICONERROR);
    }

    if (yaml_tree) {
      const std::string dsl_dump =
          auralite::ui::ViewFactory::DumpTree(dsl_tree.get());
      const std::string yaml_dump =
          auralite::ui::ViewFactory::DumpTree(yaml_tree.get());
      const bool same_tree = (dsl_dump == yaml_dump);
      if (status_ptr) {
        status_ptr->text(same_tree
                             ? L"YAML↔DSL dump OK · click Mini Button below"
                             : L"YAML↔DSL dump MISMATCH");
      }
      root->AddChild(std::move(yaml_tree));
    }
  }

  auralite::ui::SplitView* split_ptr = nullptr;
  {
    auto left = std::make_unique<auralite::ui::Label>();
    left->text(L"Left pane\n(drag divider →)")
        .font_size(14.f)
        .align(auralite::ui::TextAlign::Center);

    auto right = std::make_unique<auralite::ui::Label>();
    right->text(L"Right pane")
        .font_size(14.f)
        .align(auralite::ui::TextAlign::Center);

    auto split = std::make_unique<auralite::ui::SplitView>();
    split_ptr = split.get();
    split->preferred_size(480.f, 110.f)
        .set_ratio(0.45f)
        .set_leading(std::move(left))
        .set_trailing(std::move(right));
    root->AddChild(std::move(split));
  }

  context_menu.on_command(
      [status_ptr, &window, split_ptr](int id) {
        if (!status_ptr) {
          return;
        }
        switch (id) {
          case 1:
            status_ptr->text(L"ContextMenu: Refresh");
            break;
          case 2:
            status_ptr->text(L"ContextMenu: About AuraLite UI");
            MessageBoxW(window.hwnd(),
                        L"AuraLite Phase 2\nSplitView + ContextMenu",
                        L"About", MB_OK | MB_ICONINFORMATION);
            break;
          case 3:
            if (split_ptr) {
              split_ptr->set_ratio(0.5f);
              // Force re-layout on next paint via invalidate; Window re-layouts
              // when size changes — nudge by re-applying current bounds.
              split_ptr->Layout(split_ptr->bounds());
            }
            status_ptr->text(L"ContextMenu: split ratio reset to 50%");
            break;
          default:
            status_ptr->text(L"ContextMenu id=" + std::to_wstring(id));
            break;
        }
        window.Invalidate();
      });

  root_ptr->set_on_context_menu(
      [&context_menu, &window](int screen_x, int screen_y) {
        context_menu.Show(window.hwnd(), screen_x, screen_y);
      });

  {
    auto label = std::make_unique<auralite::ui::Label>();
    label->text(L"Username")
        .font_size(13.f)
        .color(auralite::ColorF::FromRgb(70, 80, 95))
        .preferred_height(20.f);
    root->AddChild(std::move(label));
  }
  {
    auto field = std::make_unique<auralite::ui::TextField>();
    field->placeholder(L"Type here (ASCII + IME)…")
        .preferred_size(320.f, 36.f)
        .on_change([status_ptr, &window](const std::wstring& t) {
          if (status_ptr) {
            status_ptr->text(L"Username: " + t);
          }
          window.Invalidate();
        });
    root->AddChild(std::move(field));
  }

  {
    auto label = std::make_unique<auralite::ui::Label>();
    label->text(L"Password")
        .font_size(13.f)
        .color(auralite::ColorF::FromRgb(70, 80, 95))
        .preferred_height(20.f);
    root->AddChild(std::move(label));
  }
  {
    auto field = std::make_unique<auralite::ui::TextField>();
    field->password(true)
        .placeholder(L"Password (no copy)")
        .preferred_size(320.f, 36.f);
    root->AddChild(std::move(field));
  }

  {
    auto list_label = std::make_unique<auralite::ui::Label>();
    list_label->text(L"Scrollable list (30 items)")
        .font_size(13.f)
        .color(auralite::ColorF::FromRgb(70, 80, 95))
        .preferred_height(20.f);
    root->AddChild(std::move(list_label));
  }
  {
    auto list = std::make_unique<auralite::ui::ListView>();
    for (int i = 1; i <= 30; ++i) {
      wchar_t buf[64];
      swprintf_s(buf, L"Item %02d — AuraLite ListView", i);
      list->AddItem(buf);
    }
    list->on_selection_changed([status_ptr, &window](int index) {
      if (status_ptr) {
        status_ptr->text(L"List selected: " + std::to_wstring(index));
      }
      window.Invalidate();
    });
    list->set_selected_index(0);

    auto scroll = std::make_unique<auralite::ui::ScrollView>();
    scroll->preferred_size(400.f, 180.f).set_content(std::move(list));
    root->AddChild(std::move(scroll));
  }

  {
    auto check = std::make_unique<auralite::ui::Checkbox>();
    check->text(L"Enable option")
        .on_changed([status_ptr, &window](bool checked) {
          if (status_ptr) {
            status_ptr->text(checked ? L"Checkbox: on" : L"Checkbox: off");
          }
          window.Invalidate();
        });
    root->AddChild(std::move(check));
  }

  {
    auto radio_row = std::make_unique<auralite::ui::Row>();
    radio_row->spacing(16.f);
    {
      auto radio_a = std::make_unique<auralite::ui::Radio>();
      radio_a->text(L"Option A")
          .group_id(1)
          .checked(true)
          .on_changed([status_ptr, &window](bool checked) {
            if (checked && status_ptr) {
              status_ptr->text(L"Radio: A");
            }
            window.Invalidate();
          });
      radio_row->AddChild(std::move(radio_a));
    }
    {
      auto radio_b = std::make_unique<auralite::ui::Radio>();
      radio_b->text(L"Option B")
          .group_id(1)
          .on_changed([status_ptr, &window](bool checked) {
            if (checked && status_ptr) {
              status_ptr->text(L"Radio: B");
            }
            window.Invalidate();
          });
      radio_row->AddChild(std::move(radio_b));
    }
    root->AddChild(std::move(radio_row));
  }

  {
    auto sw = std::make_unique<auralite::ui::Switch>();
    sw->text(L"Notifications")
        .on_changed([status_ptr, &window](bool on) {
          if (status_ptr) {
            status_ptr->text(on ? L"Switch: on" : L"Switch: off");
          }
          window.Invalidate();
        });
    root->AddChild(std::move(sw));
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
  window.FocusNext(false);  // first focusable = username TextField

  ShowWindow(window.hwnd(), show);
  UpdateWindow(window.hwnd());

  const int code = auralite::ui::Application::Run();
  CoUninitialize();
  return code;
}
