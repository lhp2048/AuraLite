#include <windows.h>
#include <objbase.h>

#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

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

bool UseFluent(LPWSTR cmd_line) {
  return cmd_line && wcsstr(cmd_line, L"--fluent") != nullptr;
}

std::wstring ExeDir() {
  wchar_t module[MAX_PATH] = {};
  GetModuleFileNameW(nullptr, module, MAX_PATH);
  return std::filesystem::path(module).parent_path().wstring();
}

std::string NarrowPath(const std::wstring& wide) {
  if (wide.empty()) {
    return {};
  }
  const int n = WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                                    static_cast<int>(wide.size()), nullptr, 0,
                                    nullptr, nullptr);
  if (n <= 0) {
    return {};
  }
  std::string out(static_cast<size_t>(n), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                      out.data(), n, nullptr, nullptr);
  return out;
}

std::string ResolveGalleryYaml() {
  namespace fs = std::filesystem;
  const std::wstring beside = ExeDir() + L"\\gallery.yaml";
  if (fs::exists(beside)) {
    return NarrowPath(beside);
  }
  const char* candidates[] = {
      "gallery.yaml",
      "examples/ui_gallery/gallery.yaml",
      "../examples/ui_gallery/gallery.yaml",
      "../../examples/ui_gallery/gallery.yaml",
  };
  for (const char* c : candidates) {
    if (fs::exists(c)) {
      return c;
    }
  }
  return {};
}

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

auralite::ui::Label* FindLastLabel(auralite::ui::Node* node) {
  auralite::ui::Label* last = nullptr;
  std::function<void(auralite::ui::Node*)> walk = [&](auralite::ui::Node* n) {
    if (!n) {
      return;
    }
    if (auto* label = dynamic_cast<auralite::ui::Label*>(n)) {
      last = label;
    }
    for (const auto& child : n->children()) {
      walk(child.get());
    }
  };
  walk(node);
  return last;
}

auralite::ui::Label* FindStatusLabel(auralite::ui::Node* root) {
  if (!root || root->children().size() < 2) {
    return nullptr;
  }
  return dynamic_cast<auralite::ui::Label*>(root->children()[1].get());
}

auralite::ui::SplitView* FindSplit(auralite::ui::Node* node) {
  if (!node) {
    return nullptr;
  }
  if (auto* split = dynamic_cast<auralite::ui::SplitView*>(node)) {
    return split;
  }
  for (const auto& child : node->children()) {
    if (auto* found = FindSplit(child.get())) {
      return found;
    }
  }
  return nullptr;
}

void ApplyDemoPixels(auralite::ui::Node* node) {
  if (!node) {
    return;
  }
  if (auto* image = dynamic_cast<auralite::ui::ImageView*>(node)) {
    const auto pixels = MakeCheckerBgra(64, 8);
    image->SetPixels(64, 64, pixels.data(), 64 * 4);
  }
  if (auto* btn = dynamic_cast<auralite::ui::ImageButton*>(node)) {
    const auto pixels = MakeSolidBgra(32, 70, 160, 50);
    btn->SetPixels(32, 32, pixels.data(), 32 * 4);
  }
  for (const auto& child : node->children()) {
    ApplyDemoPixels(child.get());
  }
}

void WireInteractive(auralite::ui::Node* node, auralite::ui::Label* status,
                     auralite::ui::Window* window) {
  if (!node || !status || !window) {
    return;
  }
  if (auto* cb = dynamic_cast<auralite::ui::Checkbox*>(node)) {
    cb->on_changed([status, window](bool checked) {
      status->text(checked ? L"Checkbox: on" : L"Checkbox: off");
      window->Invalidate();
    });
  }
  if (auto* radio = dynamic_cast<auralite::ui::Radio*>(node)) {
    const std::wstring label = radio->text();
    radio->on_changed([status, window, label](bool checked) {
      if (checked) {
        status->text(L"Radio: " + label);
        window->Invalidate();
      }
    });
  }
  if (auto* sw = dynamic_cast<auralite::ui::Switch*>(node)) {
    sw->on_changed([status, window](bool on) {
      status->text(on ? L"Switch: on" : L"Switch: off");
      window->Invalidate();
    });
  }
  if (auto* list = dynamic_cast<auralite::ui::ListView*>(node)) {
    list->on_selection_changed([status, window](int index) {
      status->text(L"List selected: " + std::to_wstring(index));
      window->Invalidate();
    });
  }
  if (auto* field = dynamic_cast<auralite::ui::TextField*>(node)) {
    if (!field->is_password()) {
      field->on_change([status, window](const std::wstring& t) {
        status->text(L"TextField: " + t);
        window->Invalidate();
      });
    }
  }
  if (auto* btn = dynamic_cast<auralite::ui::Button*>(node)) {
    if (btn->text() == L"Button") {
      btn->on_click([status, window]() {
        status->text(L"Button clicked");
        window->Invalidate();
      });
    }
  }
  if (auto* img_btn = dynamic_cast<auralite::ui::ImageButton*>(node)) {
    img_btn->on_click([status, window]() {
      status->text(L"ImageButton clicked");
      window->Invalidate();
    });
  }
  for (const auto& child : node->children()) {
    WireInteractive(child.get(), status, window);
  }
}

std::unique_ptr<auralite::ui::Node> BuildFluentGallery() {
  using namespace auralite::ui::dsl;

  auto list = std::make_unique<auralite::ui::ListView>();
  for (int i = 1; i <= 15; ++i) {
    wchar_t buf[64];
    swprintf_s(buf, L"Item %02d — Gallery", i);
    list->AddItem(buf);
  }
  list->set_selected_index(0);

  return Column()
      .padding(20.f)
      .spacing(10.f)
      .child(Label().text(L"AuraLite UI Gallery").font_size(22.f))
      .child(Label()
                 .text(L"拖拽分割 · 右键菜单 · 滚动列表")
                 .font_size(13.f)
                 .preferred_height(22.f))
      .child(SplitView()
                 .preferred_size(560.f, 100.f)
                 .ratio(0.42f)
                 .leading(Label()
                              .text(L"SplitView 左")
                              .font_size(14.f)
                              .align(auralite::ui::TextAlign::Center))
                 .trailing(Label()
                               .text(L"SplitView 右")
                               .font_size(14.f)
                               .align(auralite::ui::TextAlign::Center)))
      .child(Row()
                 .spacing(16.f)
                 .child(Checkbox().text(L"记住选项"))
                 .child(Switch().text(L"通知")))
      .child(Row()
                 .spacing(16.f)
                 .child(Radio().text(L"选项 A").group_id(1).checked(true))
                 .child(Radio().text(L"选项 B").group_id(1)))
      .child(Label().text(L"TextField").font_size(13.f).preferred_height(18.f))
      .child(TextField()
                 .placeholder(L"输入文字（支持 IME）")
                 .preferred_size(360.f, 36.f))
      .child(Label().text(L"Password").font_size(13.f).preferred_height(18.f))
      .child(TextField()
                 .placeholder(L"密码（不复制明文）")
                 .password(true)
                 .preferred_size(360.f, 36.f))
      .child(Button().text(L"Button").preferred_size(140.f, 40.f))
      .child(Row()
                 .spacing(12.f)
                 .child(ImageView().preferred_size(72.f, 72.f))
                 .child(ImageButton().preferred_size(56.f, 56.f)))
      .child(Label()
                 .text(L"ScrollView + ListView")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(ScrollView()
                 .preferred_size(420.f, 160.f)
                 .content(std::unique_ptr<auralite::ui::Node>(std::move(list))))
      .child(Label()
                 .text(L"在空白处右键打开 ContextMenu")
                 .font_size(12.f)
                 .preferred_height(20.f))
      .Build();
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR cmd_line, int show) {
  auralite::ui::Application::EnableDpiAwareness();
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  const bool fluent = UseFluent(cmd_line);
  const wchar_t* title =
      fluent ? L"AuraLite UI Gallery (fluent)" : L"AuraLite UI Gallery (YAML)";

  auralite::ui::Window window;
  if (!window.Create(title, 640, 920)) {
    MessageBoxW(nullptr, L"Window / Canvas init failed", L"ui_gallery",
                MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  std::unique_ptr<auralite::ui::Node> root;
  try {
    if (fluent) {
      root = BuildFluentGallery();
    } else {
      const std::string yaml_path = ResolveGalleryYaml();
      if (yaml_path.empty()) {
        MessageBoxW(nullptr,
                    L"gallery.yaml not found beside exe or under examples/",
                    L"ui_gallery", MB_ICONERROR);
        CoUninitialize();
        return 1;
      }
      auralite::ui::HandlerMap handlers;
      // Bound after WireInteractive; keep names so YAML on_click resolves if
      // re-bound later. Empty handlers are OK — WireInteractive attaches clicks.
      auralite::ui::ViewFactory factory;
      root = factory.CreateFromYamlFile(yaml_path, handlers);

      // Verify dual-track shape against fluent.
      auto fluent_tree = BuildFluentGallery();
      const std::string yaml_dump =
          auralite::ui::ViewFactory::DumpTree(root.get());
      const std::string fluent_dump =
          auralite::ui::ViewFactory::DumpTree(fluent_tree.get());
      if (yaml_dump != fluent_dump) {
        OutputDebugStringA("ui_gallery YAML↔fluent DumpTree mismatch\n");
        OutputDebugStringA(yaml_dump.c_str());
        OutputDebugStringA(fluent_dump.c_str());
      }
    }
  } catch (const std::exception& ex) {
    MessageBoxA(nullptr, ex.what(), "ui_gallery YAML failed", MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  ApplyDemoPixels(root.get());

  auralite::ui::Label* status = FindStatusLabel(root.get());
  if (!status) {
    status = FindLastLabel(root.get());
  }
  WireInteractive(root.get(), status, &window);

  auralite::ui::SplitView* split_ptr = FindSplit(root.get());
  auralite::ui::ContextMenu context_menu;
  context_menu.AddItem(1, L"Refresh status")
      .AddSeparator()
      .AddItem(2, L"About gallery")
      .AddItem(3, L"Reset split ratio");
  context_menu.on_command([status, &window, split_ptr](int id) {
    if (!status) {
      return;
    }
    switch (id) {
      case 1:
        status->text(L"ContextMenu: Refresh");
        break;
      case 2:
        status->text(L"ContextMenu: About");
        MessageBoxW(window.hwnd(),
                    L"AuraLite Phase 2 UI Gallery\n"
                    L"Label TextField Checkbox Radio Switch\n"
                    L"ImageView ImageButton Button\n"
                    L"ScrollView ListView SplitView ContextMenu Column/Row",
                    L"About", MB_OK | MB_ICONINFORMATION);
        break;
      case 3:
        if (split_ptr) {
          split_ptr->set_ratio(0.5f);
          split_ptr->Layout(split_ptr->bounds());
        }
        status->text(L"ContextMenu: split reset");
        break;
      default:
        status->text(L"ContextMenu id=" + std::to_wstring(id));
        break;
    }
    window.Invalidate();
  });

  root->set_on_context_menu(
      [&context_menu, &window](int screen_x, int screen_y) {
        context_menu.Show(window.hwnd(), screen_x, screen_y);
      });

  if (status && !fluent) {
    status->text(L"YAML 模式 · 拖拽分割 · 右键菜单 · 滚动列表");
  }

  window.SetRoot(std::move(root));
  window.FocusNext(false);

  ShowWindow(window.hwnd(), show);
  UpdateWindow(window.hwnd());

  const int code = auralite::ui::Application::Run();
  CoUninitialize();
  return code;
}
