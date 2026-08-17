#include <windows.h>
#include <objbase.h>

#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "auralite/ui/application.h"
#include "auralite/ui/button.h"
#include "auralite/ui/column.h"
#include "auralite/ui/dsl.h"
#include "auralite/ui/factory.h"
#include "auralite/ui/label.h"
#include "auralite/ui/text_field.h"
#include "auralite/ui/window.h"

namespace {

bool UseFluent(LPWSTR cmd_line) {
  if (!cmd_line) {
    return false;
  }
  return wcsstr(cmd_line, L"--fluent") != nullptr;
}

std::wstring ExeDir() {
  wchar_t module[MAX_PATH] = {};
  GetModuleFileNameW(nullptr, module, MAX_PATH);
  std::filesystem::path p(module);
  return p.parent_path().wstring();
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

std::string ResolveLoginYaml() {
  namespace fs = std::filesystem;
  const std::wstring beside = ExeDir() + L"\\login_window.yaml";
  if (fs::exists(beside)) {
    return NarrowPath(beside);
  }
  const char* candidates[] = {
      "login_window.yaml",
      "examples/login/login_window.yaml",
      "../examples/login/login_window.yaml",
      "../../examples/login/login_window.yaml",
  };
  for (const char* c : candidates) {
    if (fs::exists(c)) {
      return c;
    }
  }
  return {};
}

void CollectTextFields(auralite::ui::Node* node,
                       std::vector<auralite::ui::TextField*>* out) {
  if (!node || !out) {
    return;
  }
  if (auto* field = dynamic_cast<auralite::ui::TextField*>(node)) {
    out->push_back(field);
  }
  for (const auto& child : node->children()) {
    CollectTextFields(child.get(), out);
  }
}

auralite::ui::Label* FindStatusLabel(auralite::ui::Node* node) {
  if (!node) {
    return nullptr;
  }
  auralite::ui::Label* last = nullptr;
  std::function<void(auralite::ui::Node*)> walk =
      [&](auralite::ui::Node* n) {
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

std::unique_ptr<auralite::ui::Node> BuildFluentLogin() {
  using namespace auralite::ui::dsl;
  return Column()
      .padding(24.f)
      .spacing(12.f)
      .child(Label().text(L"Smart Family").font_size(20.f))
      .child(Label().text(L"账号").font_size(13.f).preferred_height(20.f))
      .child(TextField().placeholder(L"账号"))  // fill width, fixed height
      .child(Label().text(L"密码").font_size(13.f).preferred_height(20.f))
      .child(TextField().placeholder(L"密码").password(true))
      .child(Button().text(L"登录").is_default(true))
      .child(Label().text(L"状态：就绪").font_size(13.f).preferred_height(22.f))
      .Build();
}

bool CreateLoginWindow(auralite::ui::Window* window, const wchar_t* title) {
  if (!window) {
    return false;
  }
  // DIP outer size (padding 24*2 + field width 352 ≈ 400×360).
  return window->Create(title, 400, 360);
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR cmd_line, int show) {
  auralite::ui::Application::EnableDpiAwareness();
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  const bool fluent = UseFluent(cmd_line);
  const wchar_t* title =
      fluent ? L"AuraLite Login (fluent)" : L"AuraLite Login (YAML)";

  auralite::ui::Window window;
  if (!CreateLoginWindow(&window, title)) {
    MessageBoxW(nullptr, L"Window / Canvas init failed", L"login_demo",
                MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  std::unique_ptr<auralite::ui::Node> root;
  std::string yaml_dump;
  std::string fluent_dump;

  try {
    auto fluent_tree = BuildFluentLogin();
    fluent_dump = auralite::ui::ViewFactory::DumpTree(fluent_tree.get());

    if (fluent) {
      root = std::move(fluent_tree);
    } else {
      const std::string yaml_path = ResolveLoginYaml();
      if (yaml_path.empty()) {
        MessageBoxW(nullptr,
                    L"login_window.yaml not found beside exe or under examples/",
                    L"login_demo", MB_ICONERROR);
        CoUninitialize();
        return 1;
      }

      // Handlers filled after tree exists (need field pointers).
      auralite::ui::HandlerMap handlers;
      auralite::ui::ViewFactory factory;
      root = factory.CreateFromYamlFile(yaml_path, handlers);
      yaml_dump = auralite::ui::ViewFactory::DumpTree(root.get());
    }
  } catch (const std::exception& ex) {
    MessageBoxA(nullptr, ex.what(), "login_demo YAML failed", MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  std::vector<auralite::ui::TextField*> fields;
  CollectTextFields(root.get(), &fields);
  auralite::ui::Label* status = FindStatusLabel(root.get());
  auralite::ui::Button* login_btn = nullptr;
  for (const auto& child : root->children()) {
    if (auto* btn = dynamic_cast<auralite::ui::Button*>(child.get())) {
      login_btn = btn;
      break;
    }
  }

  auto do_login = [status, &fields, &window]() {
    const std::wstring user =
        fields.size() > 0 ? fields[0]->text() : std::wstring{};
    const std::wstring pass =
        fields.size() > 1 ? fields[1]->text() : std::wstring{};
    if (status) {
      if (user.empty()) {
        status->text(L"状态：请输入账号");
      } else if (pass.empty()) {
        status->text(L"状态：请输入密码");
      } else {
        status->text(L"状态：已登录 " + user);
      }
    }
    window.Invalidate();
  };

  if (login_btn) {
    login_btn->on_click(do_login);
  }

  // Cross-check YAML vs fluent tree shape when running YAML mode.
  if (!fluent && !yaml_dump.empty() && yaml_dump == fluent_dump && status) {
    status->text(L"状态：就绪（YAML↔fluent DumpTree OK）");
  } else if (!fluent && !yaml_dump.empty() && yaml_dump != fluent_dump &&
             status) {
    status->text(L"状态：就绪（DumpTree 形状不一致，见控制台）");
    OutputDebugStringA("YAML dump:\n");
    OutputDebugStringA(yaml_dump.c_str());
    OutputDebugStringA("Fluent dump:\n");
    OutputDebugStringA(fluent_dump.c_str());
  }

  window.SetRoot(std::move(root));
  window.FocusNext(false);

  ShowWindow(window.hwnd(), show);
  UpdateWindow(window.hwnd());

  const int code = auralite::ui::Application::Run();
  CoUninitialize();
  return code;
}
