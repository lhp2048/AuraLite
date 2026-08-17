#include <windows.h>
#include <objbase.h>

#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "auralite/ui/absolute.h"
#include "auralite/ui/application.h"
#include "auralite/ui/button.h"
#include "auralite/ui/checkbox.h"
#include "auralite/ui/column.h"
#include "auralite/ui/dsl.h"
#include "auralite/ui/factory.h"
#include "auralite/ui/popup_host.h"
#include "auralite/ui/image_button.h"
#include "auralite/ui/image_view.h"
#include "auralite/ui/label.h"
#include "auralite/ui/list_view.h"
#include "auralite/ui/radio.h"
#include "auralite/ui/row.h"
#include "auralite/ui/scroll_view.h"
#include "auralite/ui/split_view.h"
#include "auralite/ui/switch_control.h"
#include "auralite/ui/combo.h"
#include "auralite/ui/progress_bar.h"
#include "auralite/ui/slider.h"
#include "auralite/ui/text_area.h"
#include "auralite/ui/text_field.h"
#include "auralite/ui/tile.h"
#include "auralite/ui/title_bar.h"
#include "auralite/ui/toast.h"
#include "auralite/ui/item_list.h"
#include "auralite/ui/virtual_list.h"
#include "auralite/ui/tree_view.h"
#include "auralite/ui/theme.h"
#include "auralite/ui/native_host.h"
#include "auralite/ui/user_control.h"
#include "auralite/ui/window.h"
#include "auralite/ui/yaml_loader.h"
#include "auralite/canvas.h"

namespace {

struct GalleryState {
  bool animate = true;
  HWND native_demo = nullptr;
  HWND native_borrowed = nullptr;
  std::unique_ptr<auralite::ui::Window> native_win;

  ~GalleryState() {
    native_win.reset();
    if (native_borrowed && IsWindow(native_borrowed)) {
      DestroyWindow(native_borrowed);
    }
  }
};

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

std::string ResolveThemesDir() {
  namespace fs = std::filesystem;
  const std::wstring beside = ExeDir() + L"\\themes";
  if (fs::is_directory(beside)) {
    return NarrowPath(beside);
  }
  const char* candidates[] = {
      "themes",
      "examples/ui_gallery/themes",
      "../examples/ui_gallery/themes",
      "../../examples/ui_gallery/themes",
  };
  for (const char* c : candidates) {
    if (fs::is_directory(c)) {
      return c;
    }
  }
  return NarrowPath(beside);
}

void InitGalleryThemes() {
  auralite::ui::Theme::RegisterBuiltInLight();
  auralite::ui::Theme::RegisterBuiltInDark();
  const std::string dir = ResolveThemesDir();
  if (!dir.empty()) {
    auralite::ui::Theme::RegisterFromDir(dir);
  }
  auralite::ui::Theme::SetActive("light");
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

std::string ResolveGalleryFile(const char* filename) {
  namespace fs = std::filesystem;
  if (!filename || !*filename) {
    return {};
  }
  const std::wstring wide = [&] {
    std::wstring out;
    for (const char* p = filename; *p; ++p) {
      out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*p)));
    }
    return out;
  }();
  const std::wstring beside = ExeDir() + L"\\" + wide;
  if (fs::exists(beside)) {
    return NarrowPath(beside);
  }
  const std::string rels[] = {
      std::string(filename),
      std::string("examples/ui_gallery/") + filename,
      std::string("../examples/ui_gallery/") + filename,
      std::string("../../examples/ui_gallery/") + filename,
  };
  for (const auto& c : rels) {
    if (fs::exists(c)) {
      return c;
    }
  }
  return {};
}

std::string ResolvePopupMenuYaml() {
  std::string path = ResolveGalleryFile("menu_classic.yaml");
  if (!path.empty()) {
    return path;
  }
  return ResolveGalleryFile("popup_menu.yaml");
}

POINT ScreenPointBelowNode(auralite::ui::Node* node,
                           auralite::ui::Window* window) {
  POINT pt = {0, 0};
  if (!node || !window || !window->hwnd()) {
    GetCursorPos(&pt);
    return pt;
  }
  const auralite::RectF b = node->bounds();
  pt.x = static_cast<LONG>(b.x);
  pt.y = static_cast<LONG>(b.y + b.h + 4.f);
  ClientToScreen(window->hwnd(), &pt);
  return pt;
}

auralite::ui::HandlerMap MakePopupMenuHandlers(
    auralite::ui::PopupHost* host, auralite::ui::Window* window,
    auralite::ui::Label* status, auralite::ui::SplitView* split_ptr) {
  auralite::ui::HandlerMap handlers;
  handlers["refresh"] = host->WrapDismiss([status, window] {
    if (status) {
      status->text(L"PopupHost: Refresh");
    }
    window->Invalidate();
  });
  handlers["about"] = host->WrapDismiss([status, window] {
    if (status) {
      status->text(L"PopupHost: About");
    }
    MessageBoxW(window->hwnd(),
                L"AuraLite Phase 2 UI Gallery\n"
                L"Label TextField Checkbox Radio Switch\n"
                L"ImageView ImageButton Button\n"
                L"ScrollView ListView SplitView PopupHost Column/Row",
                L"About", MB_OK | MB_ICONINFORMATION);
    window->Invalidate();
  });
  handlers["split_reset"] = host->WrapDismiss([status, window, split_ptr] {
    if (split_ptr) {
      split_ptr->set_ratio(0.5f);
      split_ptr->Layout(split_ptr->bounds());
    }
    if (status) {
      status->text(L"PopupHost: split reset");
    }
    window->Invalidate();
  });
  return handlers;
}

void ShowMenuYaml(auralite::ui::PopupHost* host, auralite::ui::Window* window,
                  auralite::ui::Label* status, auralite::ui::SplitView* split,
                  const std::string& yaml_path, POINT screen,
                  const wchar_t* style_label) {
  if (!host || !window) {
    return;
  }
  if (yaml_path.empty()) {
    if (status) {
      status->text(L"菜单 YAML 未找到");
      window->Invalidate();
    }
    return;
  }
  if (status && style_label) {
    status->text(std::wstring(L"打开菜单样式: ") + style_label);
    window->Invalidate();
  }
  auto handlers = MakePopupMenuHandlers(host, window, status, split);
  host->ShowFromYaml(window->hwnd(), screen, yaml_path, handlers);
}

void WireMenuStyleButtons(auralite::ui::Node* root, auralite::ui::PopupHost* host,
                          auralite::ui::Window* window, auralite::ui::Label* status,
                          auralite::ui::SplitView* split) {
  if (!root || !host || !window) {
    return;
  }
  struct StyleBind {
    const char* name;
    const char* file;
    const wchar_t* label;
  };
  const StyleBind binds[] = {
      {"menu_style_classic", "menu_classic.yaml", L"经典扁平"},
      {"menu_style_buttons", "popup_menu.yaml", L"按钮风"},
      {"menu_style_dark", "menu_dark.yaml", L"深色"},
  };
  for (const StyleBind& b : binds) {
    auto* btn = dynamic_cast<auralite::ui::Button*>(root->FindByName(b.name));
    if (!btn) {
      continue;
    }
    const std::string path = ResolveGalleryFile(b.file);
    btn->on_click([host, window, status, split, path, btn, label = b.label] {
      ShowMenuYaml(host, window, status, split, path,
                   ScreenPointBelowNode(btn, window), label);
    });
  }
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

auralite::ui::Label* FindStatusLabel(auralite::ui::Node* node) {
  auralite::ui::Label* found = nullptr;
  std::function<void(auralite::ui::Node*)> walk = [&](auralite::ui::Node* n) {
    if (!n) {
      return;
    }
    if (n->name() == "status") {
      if (auto* label = dynamic_cast<auralite::ui::Label*>(n)) {
        found = label;
      }
    }
    for (const auto& child : n->children()) {
      walk(child.get());
    }
  };
  walk(node);
  return found;
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

auralite::ui::Tab* FindTab(auralite::ui::Node* node) {
  if (!node) {
    return nullptr;
  }
  if (auto* tab = dynamic_cast<auralite::ui::Tab*>(node)) {
    return tab;
  }
  for (const auto& child : node->children()) {
    if (auto* found = FindTab(child.get())) {
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
  if (auto* btn = dynamic_cast<auralite::ui::Button*>(node)) {
    if (btn->name() == "icon_btn") {
      const auto pixels = MakeSolidBgra(16, 40, 110, 200);
      btn->icon_bgra(16, 16, pixels.data(), 16 * 4);
    }
  }
  for (const auto& child : node->children()) {
    ApplyDemoPixels(child.get());
  }
}

void OpenGalleryDialog(auralite::ui::Window* owner, auralite::ui::Label* status,
                       const char* yaml_file) {
  auralite::ui::Window dlg;
  auralite::ui::WindowYaml spec;
  std::unique_ptr<auralite::ui::Node> root;
  const std::string yaml_path = ResolveGalleryFile(yaml_file);
  if (!yaml_path.empty()) {
    try {
      auralite::ui::HandlerMap handlers;
      handlers["dialog_close"] = [&dlg] { dlg.EndModal(IDOK); };
      auralite::ui::ViewFactory factory;
      root = factory.CreateFromYamlFile(yaml_path, handlers, &spec);
    } catch (const std::exception&) {
      root.reset();
    }
  }
  const wchar_t* title = spec.title_or(L"Dialog");
  const int dw = spec.width_or(320);
  const int dh = spec.height_or(220);
  const auto opt = spec.present ? spec.create_options(owner->hwnd())
                                : auralite::ui::Window::WindowOptions::Dialog(
                                      owner->hwnd());
  if (!dlg.Create(title, dw, dh, opt)) {
    status->text(L"Dialog create failed");
    owner->Invalidate();
    return;
  }
  if (!root) {
    auto shell = std::make_unique<auralite::ui::Column>();
    shell->fill_width().fill_height();
    auto bar = std::make_unique<auralite::ui::TitleBar>();
    bar->title(title).close(true).minimize(false).maximize(false);
    shell->AddChild(std::move(bar));
    auto col = std::make_unique<auralite::ui::Column>();
    col->padding(20.f).spacing(12.f).fill_width().fill_height();
    auto lab = std::make_unique<auralite::ui::Label>();
    lab->text(L"拖标题栏移动。对话框默认不能拖边缩放。").font_size(14.f);
    col->AddChild(std::move(lab));
    auto close = std::make_unique<auralite::ui::Button>();
    close->fixed_height(32.f).fill_width();
    close->text(L"关闭").on_click([&dlg] { dlg.EndModal(IDOK); });
    close->is_default(true);
    col->AddChild(std::move(close));
    shell->AddChild(std::move(col));
    root = std::move(shell);
  }
  dlg.SetRoot(std::move(root));
  dlg.RunModal();
  status->text(L"Dialog closed");
  owner->Invalidate();
}

void OpenModelessRounded(auralite::ui::Window* owner, auralite::ui::Label* status,
                         std::unique_ptr<auralite::ui::Window>* slot) {
  if (!owner || !status || !slot) {
    return;
  }
  if (*slot && (*slot)->hwnd()) {
    ShowWindow((*slot)->hwnd(), SW_SHOW);
    SetForegroundWindow((*slot)->hwnd());
    status->text(L"Modeless already open");
    owner->Invalidate();
    return;
  }
  if (!*slot) {
    *slot = std::make_unique<auralite::ui::Window>();
  }
  auralite::ui::Window::WindowOptions opt;
  opt.caption = false;
  opt.quit_on_close = false;
  opt.topmost = false;
  opt.owner = owner->hwnd();
  opt.center_on_owner = true;
  opt.corner_radius = 8.f;
  opt.border_width = 1.f;
  opt.resizable = true;
  auralite::ui::Window* w = slot->get();
  if (!w->Create(L"Modeless", 360, 240, opt)) {
    status->text(L"Modeless create failed");
    owner->Invalidate();
    return;
  }

  auto root = std::make_unique<auralite::ui::Column>();
  root->fill_width().fill_height();
  auto bar = std::make_unique<auralite::ui::TitleBar>();
  bar->title(L"非模态圆角窗");
  root->AddChild(std::move(bar));

  auto body = std::make_unique<auralite::ui::Column>();
  body->padding(20.f).spacing(12.f).fill_width().fill_height();
  auto hint = std::make_unique<auralite::ui::Label>();
  hint->text(L"主窗口仍可操作。拖标题栏移动，双击标题栏最大化，拖窗口边缘缩放。").font_size(14.f);
  body->AddChild(std::move(hint));
  auto close = std::make_unique<auralite::ui::Button>();
  close->text(L"关闭").variant(auralite::ui::ButtonVariant::Secondary);
  close->fill_width().fixed_height(32.f);
  close->on_click([w] { w->Close(); });
  close->is_default(true);
  body->AddChild(std::move(close));
  root->AddChild(std::move(body));
  w->SetRoot(std::move(root));
  ShowWindow(w->hwnd(), SW_SHOW);
  status->text(L"Modeless opened");
  owner->Invalidate();
}

HWND CreateDemoEdit(HWND parent) {
  HWND edit = CreateWindowExW(
      WS_EX_CLIENTEDGE, L"EDIT",
      L"Owned EDIT · 可输入。关闭本窗会 DestroyWindow。",
      WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN, 0,
      0, 10, 10, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
  if (edit) {
    SendMessageW(edit, WM_SETFONT,
                 reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
                 TRUE);
  }
  return edit;
}

HWND CreateDemoStatic() {
  HWND label = CreateWindowExW(0, L"STATIC", L"Borrowed STATIC · 关窗不销毁",
                               WS_POPUP | WS_BORDER | SS_CENTER, 0, 0, 10, 10,
                               nullptr, nullptr, GetModuleHandleW(nullptr),
                               nullptr);
  if (label) {
    SendMessageW(label, WM_SETFONT,
                 reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
                 TRUE);
  }
  return label;
}

void OpenNativeHostDemo(auralite::ui::Window* owner, auralite::ui::Label* status,
                        GalleryState* state) {
  if (!owner || !status || !state) {
    return;
  }
  if (state->native_win && state->native_win->hwnd()) {
    ShowWindow(state->native_win->hwnd(), SW_SHOW);
    SetForegroundWindow(state->native_win->hwnd());
    status->text(L"NativeHost demo already open");
    owner->Invalidate();
    return;
  }
  if (!state->native_win) {
    state->native_win = std::make_unique<auralite::ui::Window>();
  }
  auralite::ui::Window::WindowOptions opt;
  opt.caption = false;
  opt.quit_on_close = false;
  opt.topmost = false;
  opt.owner = owner->hwnd();
  opt.center_on_owner = true;
  opt.corner_radius = 8.f;
  opt.border_width = 1.f;
  opt.resizable = true;
  auralite::ui::Window* w = state->native_win.get();
  if (!w->Create(L"NativeHost", 420, 320, opt)) {
    status->text(L"NativeHost demo create failed");
    owner->Invalidate();
    return;
  }

  auto root = std::make_unique<auralite::ui::Column>();
  root->fill_width().fill_height();
  auto bar = std::make_unique<auralite::ui::TitleBar>();
  bar->title(L"NativeHost 黑盒");
  root->AddChild(std::move(bar));

  auto body = std::make_unique<auralite::ui::Column>();
  body->padding(16.f).spacing(8.f).fill_width().fill_height();
  auto hint = std::make_unique<auralite::ui::Label>();
  hint->text(L"上面 Owned（关窗销毁），下面 Borrowed（关窗保留）。")
      .font_size(13.f);
  body->AddChild(std::move(hint));

  auto owned_cap = std::make_unique<auralite::ui::Label>();
  owned_cap->text(L"Attach(hwnd) · Owned").font_size(12.f);
  body->AddChild(std::move(owned_cap));
  auto owned_host = std::make_unique<auralite::ui::NativeHost>();
  owned_host->set_name("owned_edit");
  owned_host->fill_width().fixed_height(88.f);
  body->AddChild(std::move(owned_host));

  auto borrowed_cap = std::make_unique<auralite::ui::Label>();
  borrowed_cap->text(L"AttachBorrowed(hwnd) · Borrowed").font_size(12.f);
  body->AddChild(std::move(borrowed_cap));
  auto borrowed_host = std::make_unique<auralite::ui::NativeHost>();
  borrowed_host->set_name("borrowed_static");
  borrowed_host->fill_width().fixed_height(36.f);
  body->AddChild(std::move(borrowed_host));

  auto close = std::make_unique<auralite::ui::Button>();
  close->text(L"关闭并检查 Borrowed")
      .variant(auralite::ui::ButtonVariant::Secondary);
  close->fill_width().fixed_height(32.f);
  close->is_default(true);
  close->on_click([w, owner, status, state] {
    w->Close();
    const bool alive =
        state->native_borrowed && IsWindow(state->native_borrowed);
    status->text(alive ? L"NativeHost 已关：Owned 已销毁，Borrowed 仍在"
                       : L"NativeHost 已关：Borrowed 也没了");
    owner->Invalidate();
  });
  body->AddChild(std::move(close));
  root->AddChild(std::move(body));
  w->SetRoot(std::move(root));

  if (auto* nh = dynamic_cast<auralite::ui::NativeHost*>(
          w->root()->FindByName("owned_edit"))) {
    if (HWND edit = CreateDemoEdit(w->hwnd())) {
      nh->Attach(edit);
    }
  }
  if (auto* nh = dynamic_cast<auralite::ui::NativeHost*>(
          w->root()->FindByName("borrowed_static"))) {
    if (!state->native_borrowed || !IsWindow(state->native_borrowed)) {
      state->native_borrowed = CreateDemoStatic();
    }
    if (state->native_borrowed) {
      nh->AttachBorrowed(state->native_borrowed);
    }
  }

  ShowWindow(w->hwnd(), SW_SHOW);
  status->text(L"NativeHost demo opened");
  owner->Invalidate();
}

void ApplyTreeAnimate(auralite::ui::Node* node, bool on) {
  if (!node) {
    return;
  }
  node->animate(on);
  for (const auto& child : node->children()) {
    ApplyTreeAnimate(child.get(), on);
  }
}

void WireInteractive(auralite::ui::Node* node, auralite::ui::Label* status,
                     auralite::ui::Window* window,
                     std::unique_ptr<auralite::ui::Window>* modeless,
                     GalleryState* state) {
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
    if (sw->text() == L"动画" && state) {
      sw->on_changed([status, window, state](bool on) {
        state->animate = on;
        ApplyTreeAnimate(window->root(), on);
        if (auralite::ui::Toast* t = window->toast()) {
          t->animate(on);
          window->SyncToastFade();
        }
        status->text(on ? L"动画: 开" : L"动画: 关");
        window->Invalidate();
      });
    } else {
      sw->on_changed([status, window](bool on) {
        status->text(on ? L"Switch: on" : L"Switch: off");
        window->Invalidate();
      });
    }
  }
  if (auto* list = dynamic_cast<auralite::ui::ListView*>(node)) {
    list->on_selection_changed([status, window](int index) {
      status->text(L"List selected: " + std::to_wstring(index));
      window->Invalidate();
    });
    if (list->checkable()) {
      list->on_check_changed([status, window](int index, bool checked) {
        status->text(L"List check " + std::to_wstring(index) +
                     (checked ? L": on" : L": off"));
        window->Invalidate();
      });
    }
  }
  if (auto* field = dynamic_cast<auralite::ui::TextField*>(node)) {
    if (!field->is_password()) {
      field->on_change([status, window](const std::wstring& t) {
        status->text(L"TextField: " + t);
        window->Invalidate();
      });
      field->on_submit([status, window, field]() {
        status->text(L"Submit: " + field->text());
        window->Invalidate();
      });
    }
  }
  if (auto* btn = dynamic_cast<auralite::ui::Button*>(node)) {
    const std::wstring text = btn->text();
    if (text == L"Button") {
      btn->on_click([status, window]() {
        status->text(L"Button clicked");
        window->Invalidate();
      });
    } else if (text == L"Light" || text == L"Dark") {
      btn->on_click([status, window, text]() {
        const std::string name = (text == L"Light") ? "light" : "dark";
        auralite::ui::Theme::SetActive(name);
        status->text(L"Theme: " + text);
        window->Invalidate();
      });
    } else if (text == L"Float" || text == L"RB" || text == L"left+right" ||
               text == L"A" || text == L"B" || text == L"焦1" || text == L"焦2" ||
               text == L"焦3") {
      btn->on_click([status, window, text]() {
        status->text(L"Clicked: " + text);
        window->Invalidate();
      });
    } else if (text.size() == 2 && (text[0] == L'T' || text[0] == L'G') &&
               text[1] >= L'1' && text[1] <= L'9') {
      btn->on_click([status, window, text]() {
        status->text(L"Tile: " + text);
        window->Invalidate();
      });
    } else if (text == L"图标") {
      btn->on_click([status, window]() {
        status->text(L"Icon button");
        window->Invalidate();
      });
    } else if (text == L"切换显隐") {
      btn->on_click([status, window]() {
        auralite::ui::Node* target =
            window->root() ? window->root()->FindByName("hide_me") : nullptr;
        if (auto* lab = dynamic_cast<auralite::ui::Label*>(target)) {
          lab->set_visible(!lab->visible());
          status->text(lab->visible() ? L"已显示" : L"已隐藏");
          window->RequestLayout();
        }
      });
    } else if (text == L"浮动") {
      btn->on_click([status, window]() {
        status->text(L"浮动按钮 · 仅布局页右下角");
        window->Invalidate();
      });
    } else if (text == L"全局浮层") {
      btn->on_click([status, window]() {
        status->text(L"全局浮层 · 靠右垂直居中");
        window->Invalidate();
      });
    } else if (text == L"页内浮动") {
      btn->on_click([status, window]() {
        status->text(L"页内浮动 · 仅 Tab 页面 B");
        window->Invalidate();
      });
    } else if (text == L"Open Dialog" || text == L"Open Square Dialog") {
      const char* yaml = (text == L"Open Square Dialog") ? "dialog_square.yaml"
                                                         : "dialog.yaml";
      btn->on_click([status, window, yaml]() {
        OpenGalleryDialog(window, status, yaml);
      });
    } else if (text == L"Open Modeless") {
      btn->on_click([status, window, modeless]() {
        OpenModelessRounded(window, status, modeless);
      });
    } else if (text == L"Open NativeHost") {
      btn->on_click([status, window, state]() {
        OpenNativeHostDemo(window, status, state);
      });
    } else if (text == L"Toast Info" || text == L"Toast Success" ||
               text == L"Toast Danger" || text == L"Toast Sticky") {
      btn->on_click([status, window, state, text]() {
        const bool anim = state ? state->animate : true;
        auralite::ui::ToastVariant variant =
            auralite::ui::ToastVariant::Info;
        std::wstring msg = L"提示";
        float duration = 2.5f;
        if (text == L"Toast Success") {
          variant = auralite::ui::ToastVariant::Success;
          msg = L"已保存";
        } else if (text == L"Toast Danger") {
          variant = auralite::ui::ToastVariant::Danger;
          msg = L"出错了";
        } else if (text == L"Toast Sticky") {
          duration = 0.f;
          msg = L"点我关闭";
        }
        window->ShowToast(auralite::ui::dsl::Toast()
                              .text(msg)
                              .variant(variant)
                              .duration_sec(duration)
                              .animate(anim)
                              .Build());
        status->text(text);
        window->Invalidate();
      });
    }
  }
  if (auto* tab = dynamic_cast<auralite::ui::Tab*>(node)) {
    tab->on_selected([status, window, tab](int index) {
      if (tab->name() == "gallery_nav") {
        std::wstring title = L"?";
        if (index >= 0 &&
            index < static_cast<int>(tab->headers().size())) {
          title = tab->headers()[static_cast<size_t>(index)];
        }
        status->text(L"分类: " + title);
      } else {
        status->text(L"Tab: page " + std::to_wstring(index));
      }
      window->Invalidate();
    });
  }
  if (auto* nh = dynamic_cast<auralite::ui::NativeHost*>(node)) {
    if (state && window->hwnd() && nh->name() == "native_demo") {
      if (!state->native_demo || !IsWindow(state->native_demo)) {
        state->native_demo = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT",
            L"Owned EDIT · 控件页内嵌的 Win32 输入框",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL, 0, 0, 10,
            10, window->hwnd(), nullptr, GetModuleHandleW(nullptr), nullptr);
        if (state->native_demo) {
          SendMessageW(state->native_demo, WM_SETFONT,
                       reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
                       TRUE);
        }
      }
      if (state->native_demo) {
        nh->Attach(state->native_demo);
      }
    }
  }
  if (auto* uc = dynamic_cast<auralite::ui::UserControl*>(node)) {
    uc->on_paint([](auralite::Canvas& canvas, const auralite::RectF& b) {
      const auralite::ui::ThemeTokens& th = auralite::ui::Theme::Active();
      canvas.FillRoundedRect(b, 8.f, 8.f, th.surface_alt);
      canvas.DrawText(L"UserControl · 点我", b, th.text, 14.f,
                      th.font_ui.c_str(), auralite::TextHAlign::Center);
    });
    uc->on_mouse_down([status, window](const auralite::ui::MouseEvent&) {
      status->text(L"UserControl clicked");
      window->Invalidate();
    });
  }
  if (auto* bar = dynamic_cast<auralite::ui::ProgressBar*>(node)) {
    bar->BindWindow(window);
  }
  if (auto* slider = dynamic_cast<auralite::ui::Slider*>(node)) {
    // Find sibling ProgressBar under same parent for linked demo.
    auralite::ui::ProgressBar* bar = nullptr;
    if (node->parent()) {
      for (const auto& c : node->parent()->children()) {
        if (auto* p = dynamic_cast<auralite::ui::ProgressBar*>(c.get())) {
          if (!p->indeterminate()) {
            bar = p;
            break;
          }
        }
      }
    }
    slider->on_changed([status, window, bar](float v) {
      if (bar) {
        bar->value(v);
      }
      wchar_t buf[64];
      swprintf_s(buf, L"Slider: %.2f", v);
      status->text(buf);
      window->Invalidate();
    });
  }
  if (auto* combo = dynamic_cast<auralite::ui::Combo*>(node)) {
    combo->BindWindow(window);
    if (combo->multi()) {
      combo->on_multi_changed([status, window, combo](const std::vector<int>& idxs) {
        status->text(L"Combo multi: " + std::to_wstring(idxs.size()) + L" 项");
        window->Invalidate();
        (void)combo;
      });
    } else {
      combo->on_changed([status, window, combo](int index) {
        std::wstring label = L"?";
        if (index >= 0 && index < static_cast<int>(combo->items().size())) {
          label = combo->items()[static_cast<size_t>(index)];
        }
        status->text(L"Combo: " + label);
        window->Invalidate();
      });
    }
  }
  if (auto* area = dynamic_cast<auralite::ui::TextArea*>(node)) {
    area->on_change([status, window](const std::wstring& t) {
      status->text(L"TextArea chars: " + std::to_wstring(t.size()));
      window->Invalidate();
    });
  }
  if (auto* vlist = dynamic_cast<auralite::ui::VirtualList*>(node)) {
    vlist->on_selection_changed([status, window](int index) {
      status->text(L"VirtualList: " + std::to_wstring(index));
      window->Invalidate();
    });
    vlist->on_sort_changed([status, window](int col, auralite::ui::ListSortDir dir) {
      std::wstring d = L"无";
      if (dir == auralite::ui::ListSortDir::Asc) {
        d = L"升序";
      } else if (dir == auralite::ui::ListSortDir::Desc) {
        d = L"降序";
      }
      status->text(L"VirtualList 排序 col=" + std::to_wstring(col) + L" " + d);
      window->Invalidate();
    });
    vlist->on_check_changed([status, window](int index, bool checked) {
      status->text(L"VirtualList check " + std::to_wstring(index) +
                   (checked ? L": on" : L": off"));
      window->Invalidate();
    });
  }
  if (auto* tree = dynamic_cast<auralite::ui::TreeView*>(node)) {
    tree->checkable(true);
    tree->on_selection_changed([status, window, tree](int id) {
      status->text(L"TreeView: " + tree->text(id));
      window->Invalidate();
    });
    tree->on_expanded_changed([status, window](int id, bool expanded) {
      status->text(L"TreeView id " + std::to_wstring(id) +
                   (expanded ? L" expanded" : L" collapsed"));
      window->Invalidate();
    });
    tree->on_check_changed([status, window, tree](int id, auralite::ui::TreeCheckState st) {
      std::wstring s = L"未选";
      if (st == auralite::ui::TreeCheckState::Checked) {
        s = L"已选";
      } else if (st == auralite::ui::TreeCheckState::Partial) {
        s = L"部分";
      }
      status->text(L"TreeView 勾选 " + tree->text(id) + L": " + s);
      window->Invalidate();
    });
    tree->on_load_children([status, window, tree](int id) {
      status->text(L"TreeView 懒加载: " + tree->text(id));
      // Simulate async fill (sync here for demo).
      tree->AddChild(id, L"云端灯-1");
      tree->AddChild(id, L"云端灯-2");
      tree->AddChild(id, L"云端开关");
      tree->NotifyChildrenLoaded(id);
      window->Invalidate();
    });
  }
  if (auto* items = dynamic_cast<auralite::ui::ItemList*>(node)) {
    items->row_height(40.f);
    while (items->item_count() < 100) {
      items->AddItem();
    }
    if (items->has_item_template()) {
      items->on_bind_item(
          [status, window](int index, auralite::ui::Node& row,
                           const auralite::ui::ItemListRowState& st) {
            if (auto* title =
                    dynamic_cast<auralite::ui::Label*>(row.FindByName("title"))) {
              title->text(L"任务-" + std::to_wstring(index + 1));
              title->color(st.selected ? auralite::ColorF::FromRgb(255, 255, 255)
                                       : auralite::ColorF::FromRgb(25, 35, 50));
            }
            if (auto* bar = dynamic_cast<auralite::ui::ProgressBar*>(
                    row.FindByName("progress"))) {
              const float v = static_cast<float>((index * 17) % 101) / 100.f;
              bar->value(v);
            }
            if (auto* btn = dynamic_cast<auralite::ui::Button*>(
                    row.FindByName("action"))) {
              btn->on_click([status, window, index]() {
                status->text(L"ItemList 详情: row " + std::to_wstring(index));
                window->Invalidate();
              });
            }
          });
    } else {
      items->on_paint_item(
          [](auralite::Canvas& canvas, const auralite::RectF& row,
             const auralite::ui::ItemListRowState& st) {
            const auralite::ColorF bg =
                st.selected ? auralite::ColorF::FromRgb(51, 120, 210)
                : st.hovered ? auralite::ColorF::FromRgb(230, 238, 250)
                             : auralite::ColorF::FromRgb(255, 255, 255);
            canvas.FillRect(row, bg);
            const auralite::ColorF tc =
                st.selected ? auralite::ColorF::FromRgb(255, 255, 255)
                            : auralite::ColorF::FromRgb(25, 35, 50);
            canvas.DrawText(L"item " + std::to_wstring(st.index),
                            auralite::RectF{row.x + 8.f, row.y,
                                            std::max(0.f, row.w - 16.f), row.h},
                            tc, 13.f, L"Microsoft YaHei UI",
                            auralite::TextHAlign::Left);
          });
    }
    items->on_selection_changed([status, window](int index) {
      status->text(L"ItemList: row " + std::to_wstring(index));
      window->Invalidate();
    });
    items->on_sort_changed([status, window](int col, auralite::ui::ListSortDir dir) {
      std::wstring d = L"无";
      if (dir == auralite::ui::ListSortDir::Asc) {
        d = L"升序";
      } else if (dir == auralite::ui::ListSortDir::Desc) {
        d = L"降序";
      }
      status->text(L"ItemList 排序 col=" + std::to_wstring(col) + L" " + d);
      window->Invalidate();
    });
  }
  if (auto* img_btn = dynamic_cast<auralite::ui::ImageButton*>(node)) {
    img_btn->on_click([status, window]() {
      status->text(L"ImageButton clicked");
      window->Invalidate();
    });
  }
  if (auto* lab = dynamic_cast<auralite::ui::Label*>(node)) {
    if (lab->text() == L"拖我") {
      lab->draggable(true).drag_data(L"chip");
    }
    if (lab->text() == L"放到这里") {
      lab->on_drop([status, window](const auralite::ui::DragEvent& e) {
        status->text(L"收到: " + e.data);
        window->Invalidate();
      });
    }
  }
  for (const auto& child : node->children()) {
    WireInteractive(child.get(), status, window, modeless, state);
  }
}

std::unique_ptr<auralite::ui::Node> MakeDemoVirtualList() {
  using namespace auralite::ui::dsl;
  constexpr int kCount = 50000;
  auto checked = std::make_shared<std::vector<char>>(
      static_cast<size_t>(kCount), 0);

  return VirtualList()
      .fixed_height(180.f)
      .show_header(true)
      .frozen_count(1)
      .columns({{L"名称", 140.f, 0.f, auralite::ui::TextAlign::Left},
                {L"状态", 160.f, 0.f, auralite::ui::TextAlign::Left},
                {L"详情", 180.f, 0.f, auralite::ui::TextAlign::Left},
                {L"编号", 100.f, 0.f, auralite::ui::TextAlign::Right}})
      .item_count([kCount]() { return kCount; })
      .item_kind([](int i) {
        using K = auralite::ui::VirtualListItemKind;
        switch (i % 5) {
          case 1:
            return K::IconText;
          case 2:
            return K::TwoLine;
          case 3:
            return K::Checkable;
          case 4:
            return K::Custom;
          default:
            return K::Text;
        }
      })
      .item_text([](int i) {
        return L"行 " + std::to_wstring(i);
      })
      .item_cell_text([](int i, int col) {
        if (col == 0) {
          return L"行 " + std::to_wstring(i + 1);
        }
        if (col == 1) {
          return L"状态-" + std::to_wstring(i % 5);
        }
        if (col == 2) {
          return L"详情-" + std::to_wstring(i);
        }
        return L"#" + std::to_wstring(i);
      })
      .item_sub_text([](int i) {
        return L"副标题 · index=" + std::to_wstring(i);
      })
      .item_checked([checked](int i) {
        return i >= 0 && i < static_cast<int>(checked->size()) &&
               (*checked)[static_cast<size_t>(i)] != 0;
      })
      .item_set_checked([checked](int i, bool on) {
        if (i >= 0 && i < static_cast<int>(checked->size())) {
          (*checked)[static_cast<size_t>(i)] = on ? 1 : 0;
        }
      })
      .on_paint_item([](auralite::Canvas& canvas, const auralite::RectF& row,
                        const auralite::ui::VirtualListItemState& st) {
        if (st.index % 5 != 4) {
          return;  // non-Custom: default template already painted
        }
        const auralite::ColorF bg =
            st.selected ? auralite::ColorF::FromRgb(51, 120, 210)
            : st.hovered ? auralite::ColorF::FromRgb(230, 238, 250)
                         : auralite::ColorF::FromRgb(255, 248, 240);
        canvas.FillRect(row, bg);
        const auralite::ColorF tc =
            st.selected ? auralite::ColorF::FromRgb(255, 255, 255)
                        : auralite::ColorF::FromRgb(25, 35, 50);
        const auralite::RectF title_r{row.x + 8.f, row.y, row.w - 64.f, row.h};
        canvas.DrawText(L"自定义行 " + std::to_wstring(st.index), title_r, tc,
                        14.f, L"Microsoft YaHei UI",
                        auralite::TextHAlign::Left);
        const float bw = 48.f;
        const auralite::RectF badge{row.x + row.w - bw - 8.f, row.y + 8.f, bw,
                                    row.h - 16.f};
        canvas.FillRoundedRect(badge, 4.f, 4.f,
                               auralite::ColorF::FromRgb(220, 90, 70));
        canvas.DrawText(L"CUS", badge, auralite::ColorF::FromRgb(255, 255, 255),
                        11.f, L"Microsoft YaHei UI",
                        auralite::TextHAlign::Center);
      })
      .Build();
}

std::unique_ptr<auralite::ui::Node> MakeDemoTreeView() {
  using namespace auralite::ui::dsl;
  auto tree = TreeView().checkable(true).fixed_height(180.f).Build();
  auto* t = static_cast<auralite::ui::TreeView*>(tree.get());
  const int root = t->AddRoot(L"智能家庭", true);
  const int rooms = t->AddChild(root, L"房间", true);
  t->AddChild(rooms, L"客厅");
  t->AddChild(rooms, L"主卧");
  t->AddChild(rooms, L"次卧");
  const int devices = t->AddChild(root, L"设备", true);
  const int lights = t->AddChild(devices, L"灯光", false);
  t->AddChild(lights, L"吊灯");
  t->AddChild(lights, L"筒灯");
  t->AddChild(devices, L"空调");
  t->AddChild(devices, L"窗帘");
  const int scenes = t->AddChild(root, L"情景", false);
  t->AddChild(scenes, L"回家");
  t->AddChild(scenes, L"离家");
  const int cloud = t->AddChild(root, L"云端设备", false);
  t->set_lazy(cloud, true);
  t->set_checked(root, auralite::ui::TreeCheckState::Checked, false);
  t->set_selected_id(rooms, false);
  return tree;
}

std::unique_ptr<auralite::ui::Node> MakeDemoItemList() {
  using namespace auralite::ui::dsl;
  auto list = ItemList().row_height(40.f).fixed_height(160.f).Build();
  auto* items = static_cast<auralite::ui::ItemList*>(list.get());
  items->item_template_factory([]() {
    auto title = Label().text(L"").Build();
    title->set_name("title");
    title->fixed_width(140.f);
    auto bar = ProgressBar().value(0.f).Build();
    bar->set_name("progress");
    bar->fixed_height(10.f);
    bar->fill_width();
    bar->weight(1.f);
    auto action = Button().text(L"详情").Build();
    action->set_name("action");
    action->fixed_width(64.f);
    action->fixed_height(28.f);
    return Row()
        .spacing(8.f)
        .fill_width()
        .fixed_height(36.f)
        .child(std::move(title))
        .child(std::move(bar))
        .child(std::move(action))
        .Build();
  });
  for (int i = 0; i < 100; ++i) {
    items->AddItem();
  }
  return list;
}

std::unique_ptr<auralite::ui::Node> GalleryScrollPage(
    std::unique_ptr<auralite::ui::Node> inner) {
  using namespace auralite::ui::dsl;
  return ScrollView()
      .fill_width()
      .fill_height()
      .content(std::move(inner))
      .Build();
}

std::unique_ptr<auralite::ui::Node> MakeLayoutPage();

std::unique_ptr<auralite::ui::Node> GalleryLayoutTabPage() {
  using namespace auralite::ui::dsl;
  auto fab = Button()
                 .text(L"浮动")
                 .hug_width()
                 .fixed_height(40.f)
                 .right(16.f)
                 .bottom(16.f)
                 .Build();
  fab->tooltip(L"仅「布局」页右下角，切走即隐藏");
  return Absolute()
      .fill_width()
      .fill_height()
      .child(GalleryScrollPage(MakeLayoutPage()))
      .child(std::move(fab))
      .Build();
}

std::unique_ptr<auralite::ui::Node> MakeHugButton(const wchar_t* text,
                                                 const wchar_t* tip) {
  using namespace auralite::ui::dsl;
  auto b = Button().text(text).hug_width().fixed_height(32.f).Build();
  if (tip && tip[0] != 0) {
    b->tooltip(tip);
  }
  return b;
}

std::unique_ptr<auralite::ui::Node> MakeControlsPage() {
  using namespace auralite::ui::dsl;
  auto icon_btn = Button()
                      .name("icon_btn")
                      .text(L"图标")
                      .hug_width()
                      .fixed_height(36.f)
                      .Build();
  icon_btn->tooltip(L"Button::icon_bgra");
  return Column()
      .padding(16.f)
      .spacing(10.f)
      .child(Row()
                 .spacing(16.f)
                 .child(Checkbox().text(L"记住选项"))
                 .child(Switch().text(L"通知")))
      .child(Row()
                 .spacing(16.f)
                 .child(Radio().text(L"选项 A").group_id(1).checked(true))
                 .child(Radio().text(L"选项 B").group_id(1)))
      .child(Label().text(L"TextField").font_size(13.f).preferred_height(18.f))
      .child(TextField().placeholder(L"输入文字（支持 IME）"))
      .child(Label().text(L"Password").font_size(13.f).preferred_height(18.f))
      .child(TextField().placeholder(L"密码（不复制明文）").password(true))
      .child(Label()
                 .text(L"TextField 回车提交")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(TextField().placeholder(L"回车提交"))
      .child(Button().text(L"Button"))
      .child(Row()
                 .spacing(8.f)
                 .child(Button().text(L"Primary").fixed_height(36.f))
                 .child(Button()
                            .text(L"Secondary")
                            .variant(auralite::ui::ButtonVariant::Secondary)
                            .fixed_height(36.f))
                 .child(Button()
                            .text(L"Danger")
                            .variant(auralite::ui::ButtonVariant::Danger)
                            .fixed_height(36.f)))
      .child(Row()
                 .spacing(8.f)
                 .child(Button()
                            .text(L"禁用")
                            .enabled(false)
                            .hug_width()
                            .fixed_height(36.f))
                 .child(std::move(icon_btn)))
      .child(Row()
                 .spacing(12.f)
                 .child(ImageView().preferred_size(72.f, 72.f))
                 .child(ImageButton().name("img_on").preferred_size(56.f, 56.f))
                 .child(ImageButton()
                            .name("img_off")
                            .preferred_size(56.f, 56.f)
                            .enabled(false)))
      .child(Label()
                 .text(L"ProgressBar / Slider")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(ProgressBar().value(0.45f).fixed_height(12.f))
      .child(Slider().value(0.45f).step(0.05f).tick_count(5))
      .child(Label()
                 .text(L"ProgressBar indeterminate")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(ProgressBar().indeterminate(true).fixed_height(12.f))
      .child(Label()
                 .text(L"Vertical Slider")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Row()
                 .spacing(12.f)
                 .fixed_height(100.f)
                 .child(Slider()
                            .value(0.6f)
                            .orientation(auralite::ui::SliderOrientation::Vertical)
                            .fixed_width(28.f)
                            .fill_height())
                 .child(Label()
                            .text(L"↑ 键盘 / 拖动")
                            .font_size(12.f)
                            .preferred_height(20.f)))
      .child(Label().text(L"Combo").font_size(13.f).preferred_height(18.f))
      .child(Combo()
                 .items({L"选项一", L"选项二", L"选项三", L"选项四", L"选项五"})
                 .selected(0))
      .child(Label()
                 .text(L"Combo（可筛选）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Combo()
                 .editable(true)
                 .items({L"北京", L"上海", L"广州", L"深圳", L"杭州", L"成都"})
                 .selected(0))
      .child(Label()
                 .text(L"Combo（多选）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Combo()
                 .multi(true)
                 .items({L"红", L"橙", L"黄", L"绿", L"蓝"})
                 .selected_indices({0, 2}))
      .child(Label()
                 .text(L"TextArea（软换行）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(TextArea()
                 .placeholder(L"多行输入（软换行 / Enter 硬换行）")
                 .wrap(true)
                 .text(L"这是一段较长的文本，用来演示按控件宽度自动软换行；也可以按 Enter 插入硬换行。\n第二段。")
                 .fixed_height(100.f))
      .child(Label()
                 .text(L"TextArea（不换行）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(TextArea()
                 .placeholder(L"关闭软换行")
                 .wrap(false)
                 .text(L"关闭软换行后这一行会保持很长：ABCDEFGHIJKLMNOPQRSTUVWXYZ-0123456789")
                 .fixed_height(72.f))
      .child(Label()
                 .text(L"Label wrap / trim")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Label()
                 .wrap(true)
                 .font_size(14.f)
                 .text(L"这段 Label 会按列宽自动折行（只读）。单行过长可用 trim: end / middle / start。"))
      .child(Row()
                 .spacing(8.f)
                 .child(Label()
                            .text(L"ABCDEFGHIJKLMNOPQRSTUVWXYZ")
                            .trim(auralite::ui::TextTrim::End)
                            .fixed_width(120.f)
                            .fixed_height(22.f))
                 .child(Label()
                            .text(L"C:\\very\\long\\path\\file.txt")
                            .trim(auralite::ui::TextTrim::Middle)
                            .fixed_width(160.f)
                            .fixed_height(22.f)))
      .child(Label()
                 .text(L"Tab / Shift+Tab 走焦")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Row()
                 .spacing(8.f)
                 .child(Button().text(L"焦1").hug_width().fixed_height(32.f))
                 .child(Button().text(L"焦2").hug_width().fixed_height(32.f))
                 .child(Button().text(L"焦3").hug_width().fixed_height(32.f)))
      .child(Label()
                 .text(L"UserControl（自绘）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(UserControl().name("demo_canvas").fill_width().fixed_height(80.f))
      .child(Label()
                 .text(L"NativeHost（HWND 黑盒 · Owned EDIT）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(NativeHost().name("native_demo").fill_width().fixed_height(80.f))
      .Build();
}

std::unique_ptr<auralite::ui::Node> MakeLayoutPage() {
  using namespace auralite::ui::dsl;
  auto float_label =
      Label().text(L"x=12 y=48").font_size(13.f).Build();
  float_label->hug_width();
  float_label->hug_height();
  float_label->set_pos(12.f, 48.f);
  return Column()
      .padding(16.f)
      .spacing(10.f)
      .child(Label()
                 .text(L"拖拽分割 · SplitView")
                 .font_size(13.f)
                 .preferred_height(22.f))
      .child(SplitView()
                 .fill_width()
                 .fixed_height(100.f)
                 .ratio(0.42f)
                 .leading(Label()
                              .text(L"SplitView 左")
                              .font_size(14.f)
                              .align(auralite::ui::TextAlign::Center))
                 .trailing(Label()
                               .text(L"SplitView 右")
                               .font_size(14.f)
                               .align(auralite::ui::TextAlign::Center)))
      .child(Label()
                 .text(L"weight + h_align / v_align（仅 fill 吃 weight）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Row()
                 .spacing(8.f)
                 .v_align(auralite::ui::Align::Center)
                 .fixed_height(48.f)
                 .child(Button().text(L"w1").weight(1.f).fixed_height(32.f))
                 .child(Button().text(L"w2").weight(2.f).fixed_height(40.f))
                 .child(Button().text(L"hug").hug_width().fixed_height(28.f)))
      .child(Column()
                 .spacing(6.f)
                 .h_align(auralite::ui::Align::End)
                 .fixed_height(100.f)
                 .child(Button()
                            .text(L"上 weight1")
                            .weight(1.f)
                            .fill_height()
                            .fixed_width(120.f))
                 .child(Button()
                            .text(L"下 weight2")
                            .weight(2.f)
                            .fill_height()
                            .fixed_width(160.f)))
      .child(Label()
                 .text(L"h_align center（Row 无 fill 子项时水平打包）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Row()
                 .spacing(8.f)
                 .h_align(auralite::ui::Align::Center)
                 .fixed_height(40.f)
                 .child(Button().text(L"A").hug_width().fixed_height(28.f))
                 .child(Button().text(L"B").hug_width().fixed_height(28.f)))
      .child(Label()
                 .text(L"Tile（固定 3 列）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Tile()
                 .columns(3)
                 .item_size(80.f, 36.f)
                 .spacing(8.f)
                 .child(Button().text(L"T1").preferred_size(80.f, 36.f))
                 .child(Button().text(L"T2").preferred_size(80.f, 36.f))
                 .child(Button().text(L"T3").preferred_size(80.f, 36.f))
                 .child(Button().text(L"T4").preferred_size(80.f, 36.f))
                 .child(Button().text(L"T5").preferred_size(80.f, 36.f))
                 .child(Button().text(L"T6").preferred_size(80.f, 36.f)))
      .child(Label()
                 .text(L"Tile（columns=0 按宽度折列）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Tile()
                 .columns(0)
                 .item_size(80.f, 36.f)
                 .spacing(8.f)
                 .child(Button().text(L"G1").preferred_size(80.f, 36.f))
                 .child(Button().text(L"G2").preferred_size(80.f, 36.f))
                 .child(Button().text(L"G3").preferred_size(80.f, 36.f))
                 .child(Button().text(L"G4").preferred_size(80.f, 36.f))
                 .child(Button().text(L"G5").preferred_size(80.f, 36.f)))
      .child(Label()
                 .text(L"Tab（子页里的 Absolute 叠层）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Tab()
                 .selected(0)
                 .headers({L"页面 A", L"页面 B"})
                 .header_height(36.f)
                 .fill_width()
                 .fixed_height(160.f)
                 .child(Label()
                            .text(L"页面 A：没有浮动层")
                            .font_size(14.f)
                            .align(auralite::ui::TextAlign::Center))
                 .child(Absolute()
                            .fill_width()
                            .fill_height()
                            .child(Label()
                                       .text(L"页面 B：右下角浮动，切回 A 即隐藏")
                                       .font_size(14.f)
                                       .align(auralite::ui::TextAlign::Center))
                            .child(Button()
                                       .text(L"页内浮动")
                                       .hug_width()
                                       .fixed_height(32.f)
                                       .right(8.f)
                                       .bottom(8.f))))
      .child(Label()
                 .text(L"Absolute（锚定优先 / x·y 兜底）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Absolute()
                 .fill_width()
                 .fixed_height(120.f)
                 .child(Button()
                            .text(L"left+right")
                            .left(8.f)
                            .right(8.f)
                            .top(8.f)
                            .fixed_height(32.f))
                 .child(std::move(float_label))
                 .child(Button()
                            .text(L"RB")
                            .right(8.f)
                            .bottom(8.f)
                            .fixed_width(72.f)
                            .fixed_height(28.f)))
      .child(Label().text(L"set_visible").font_size(13.f).preferred_height(18.f))
      .child(Row()
                 .spacing(8.f)
                 .v_align(auralite::ui::Align::Center)
                 .child(Button().text(L"切换显隐").hug_width().fixed_height(32.f))
                 .child(Label()
                            .name("hide_me")
                            .text(L"这段可以藏起来")
                            .font_size(13.f)
                            .preferred_height(22.f)))
      .Build();
}

std::unique_ptr<auralite::ui::Node> MakeListsPage() {
  using namespace auralite::ui::dsl;
  auto list = std::make_unique<auralite::ui::ListView>();
  for (int i = 1; i <= 15; ++i) {
    wchar_t buf[64];
    swprintf_s(buf, L"Item %02d — Gallery", i);
    list->AddItem(buf);
  }
  list->set_selected_index(0);

  auto checks = std::make_unique<auralite::ui::ListView>();
  checks->checkable(true);
  checks->AddItem(L"可选 A");
  checks->AddItem(L"可选 B");
  checks->AddItem(L"可选 C");
  checks->AddItem(L"可选 D");
  checks->set_selected_index(0);
  checks->set_checked_indices({0, 2});

  return Column()
      .padding(16.f)
      .spacing(10.f)
      .child(Label()
                 .text(L"VirtualList（多列表头）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(MakeDemoVirtualList())
      .child(Label()
                 .text(L"TreeView（勾选 + 懒加载）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(MakeDemoTreeView())
      .child(Label()
                 .text(L"ItemList（模板 + 表头）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(MakeDemoItemList())
      .child(Label()
                 .text(L"ScrollView + ListView")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(ScrollView()
                 .fill_width()
                 .fixed_height(160.f)
                 .content(std::unique_ptr<auralite::ui::Node>(std::move(list))))
      .child(Label()
                 .text(L"ListView（勾选）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(ScrollView()
                 .fill_width()
                 .fixed_height(140.f)
                 .content(std::unique_ptr<auralite::ui::Node>(std::move(checks))))
      .Build();
}

std::unique_ptr<auralite::ui::Node> MakeWindowPage() {
  using namespace auralite::ui::dsl;
  auto clipped_inner =
      Column().child(Label().text(L"clipped")).Build();
  clipped_inner->bg(auralite::ColorF::FromRgb(0xc8, 0x50, 0x50));
  clipped_inner->fill_width();
  clipped_inner->fixed_height(80.f);
  auto clipped_col = Column().child(std::move(clipped_inner)).Build();
  clipped_col->fixed_width(140.f);
  clipped_col->fixed_height(48.f);

  auto overflow_inner =
      Column().child(Label().text(L"visible overflow")).Build();
  overflow_inner->bg(auralite::ColorF::FromRgb(0x50, 0xa0, 0x50));
  overflow_inner->fill_width();
  overflow_inner->fixed_height(80.f);
  auto overflow_col = Column().child(std::move(overflow_inner)).Build();
  overflow_col->fixed_width(140.f);
  overflow_col->fixed_height(48.f);
  overflow_col->clip_children(false);

  return Column()
      .padding(16.f)
      .spacing(10.f)
      .child(Label()
                 .text(L"裁剪 / Tooltip / Dialog / Toast")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Label()
                 .text(L"对话框 Esc 关闭；Open Modeless 拖边缩放；Open NativeHost 嵌 HWND")
                 .font_size(12.f)
                 .preferred_height(20.f))
      .child(Row()
                 .spacing(12.f)
                 .child(std::move(clipped_col))
                 .child(std::move(overflow_col)))
      .child(Row()
                 .spacing(8.f)
                 .child(MakeHugButton(L"Open Dialog", L"圆角无边框对话框"))
                 .child(MakeHugButton(L"Open Square Dialog", L"直角无边框对话框"))
                 .child(MakeHugButton(L"Open Modeless", L"非模态圆角窗：拖标题栏移动，双击最大化，拖边缩放"))
                 .child(MakeHugButton(L"Open NativeHost", L"嵌 Win32 EDIT/STATIC：Owned 关窗销毁，Borrowed 保留")))
      .child(Label()
                 .text(L"Toast 浮层（可连点排队）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Row()
                 .spacing(8.f)
                 .child(MakeHugButton(L"Toast Info", L"Info 浮层"))
                 .child(MakeHugButton(L"Toast Success", L"Success 浮层"))
                 .child(MakeHugButton(L"Toast Danger", L"Danger 浮层"))
                 .child(MakeHugButton(L"Toast Sticky", L"duration<=0，点击关闭")))
      .child(Label()
                 .text(L"页内 Toast（树内节点，非浮层）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Toast()
                 .text(L"页内 Toast（非浮层）")
                 .variant(auralite::ui::ToastVariant::Info)
                 .fill_width()
                 .fixed_height(36.f))
      .child(Label()
                 .text(L"Popup 菜单样式（点按钮试开）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Row()
                 .spacing(8.f)
                 .child(Button()
                            .name("menu_style_classic")
                            .text(L"经典扁平")
                            .hug_width()
                            .fixed_height(32.f))
                 .child(Button()
                            .name("menu_style_buttons")
                            .text(L"按钮风")
                            .hug_width()
                            .fixed_height(32.f))
                 .child(Button()
                            .name("menu_style_dark")
                            .text(L"深色")
                            .hug_width()
                            .fixed_height(32.f)))
      .child(Label()
                 .text(L"空白处右键 = 经典菜单（menu_classic.yaml）")
                 .font_size(12.f)
                 .preferred_height(20.f))
      .Build();
}

std::unique_ptr<auralite::ui::Node> MakeDragPage() {
  using namespace auralite::ui::dsl;
  return Column()
      .padding(16.f)
      .spacing(10.f)
      .child(Label()
                 .text(L"从资源管理器拖文件到本窗口（任意页签都接收）")
                 .font_size(13.f)
                 .preferred_height(22.f))
      .child(Label()
                 .text(L"节点拖放（进程内，不是 OLE）")
                 .font_size(13.f)
                 .preferred_height(18.f))
      .child(Row()
                 .spacing(12.f)
                 .child(Label()
                            .text(L"拖我")
                            .font_size(13.f)
                            .preferred_height(22.f))
                 .child(Label()
                            .text(L"放到这里")
                            .font_size(13.f)
                            .preferred_height(22.f)))
      .Build();
}

std::unique_ptr<auralite::ui::Node> BuildFluentGallery() {
  using namespace auralite::ui::dsl;

  auto anim_sw = Switch().text(L"动画").on(true).Build();
  anim_sw->tooltip(
      L"开关控件过渡：Toast / Switch / Slider / Tab / 滚动 / Tooltip");

  auto global_fab = Button()
                        .text(L"全局浮层")
                        .hug_width()
                        .fixed_height(72.f)
                        .right(12.f)
                        .v_align(auralite::ui::Align::Center)
                        .bg(auralite::ColorF::FromRgb(40, 110, 200, 128))
                        .bg_hover(auralite::ColorF::FromRgb(55, 130, 215, 128))
                        .bg_pressed(auralite::ColorF::FromRgb(25, 85, 160, 160))
                        .text_color(auralite::ColorF::FromRgb(255, 255, 255))
                        .Build();
  global_fab->tooltip(
      L"全局浮层：靠右垂直居中，切页仍在；底 50% 透明");

  return Absolute()
      .fill_width()
      .fill_height()
      .child(Column()
                 .fill_width()
                 .fill_height()
                 .padding(12.f)
                 .spacing(8.f)
                 .child(Label().text(L"AuraLite UI Gallery").font_size(22.f))
                 .child(Row()
                            .spacing(8.f)
                            .v_align(auralite::ui::Align::Center)
                            .child(Button()
                                       .text(L"Light")
                                       .hug_width()
                                       .fixed_height(32.f))
                            .child(Button()
                                       .text(L"Dark")
                                       .hug_width()
                                       .fixed_height(32.f))
                            .child(std::move(anim_sw)))
                 .child(Label()
                            .name("status")
                            .text(L"就绪 · 从资源管理器拖文件到窗口")
                            .font_size(13.f)
                            .preferred_height(22.f))
                 .child(Tab()
                            .name("gallery_nav")
                            .selected(0)
                            .headers({L"控件", L"布局", L"列表", L"窗口", L"拖放"})
                            .header_height(36.f)
                            .fill_width()
                            .fill_height()
                            .child(GalleryScrollPage(MakeControlsPage()))
                            .child(GalleryLayoutTabPage())
                            .child(GalleryScrollPage(MakeListsPage()))
                            .child(GalleryScrollPage(MakeWindowPage()))
                            .child(GalleryScrollPage(MakeDragPage()))))
      .child(std::move(global_fab))
      .Build();
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR cmd_line, int show) {
  auralite::ui::Application::EnableDpiAwareness();
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  InitGalleryThemes();

  const bool fluent = UseFluent(cmd_line);
  const bool check_only = cmd_line && wcsstr(cmd_line, L"--check") != nullptr;
  const wchar_t* title =
      fluent ? L"AuraLite UI Gallery (fluent)" : L"AuraLite UI Gallery (YAML)";

  auto write_dump_check = [](bool ok, const std::string& yaml_dump,
                             const std::string& fluent_dump) {
    std::string path;
    const std::string yaml_path = ResolveGalleryYaml();
    if (!yaml_path.empty()) {
      const auto slash = yaml_path.find_last_of("/\\");
      path = (slash == std::string::npos)
                 ? std::string("gallery_dump_check.txt")
                 : yaml_path.substr(0, slash + 1) + "gallery_dump_check.txt";
    } else {
      path = "gallery_dump_check.txt";
    }
    std::ofstream out(path);
    if (!out) {
      return;
    }
    out << (ok ? "OK\n" : "MISMATCH\n");
    if (!ok) {
      out << "=== YAML ===\n" << yaml_dump << "\n=== fluent ===\n"
          << fluent_dump;
    }
  };

  if (check_only) {
    int code = 1;
    try {
      const std::string yaml_path = ResolveGalleryYaml();
      if (yaml_path.empty()) {
        CoUninitialize();
        return 1;
      }
      auralite::ui::ViewFactory factory;
      auralite::ui::HandlerMap handlers;
      auto yaml_tree = factory.CreateFromYamlFile(yaml_path, handlers);
      auto fluent_tree = BuildFluentGallery();
      const std::string yaml_dump =
          auralite::ui::ViewFactory::DumpTree(yaml_tree.get());
      const std::string fluent_dump =
          auralite::ui::ViewFactory::DumpTree(fluent_tree.get());
      const bool ok = yaml_dump == fluent_dump;
      write_dump_check(ok, yaml_dump, fluent_dump);
      code = ok ? 0 : 1;
    } catch (const std::exception& ex) {
      std::ofstream out("gallery_dump_check.txt");
      if (out) {
        out << "EXCEPTION\n" << ex.what();
      }
      write_dump_check(false, std::string("EXCEPTION: ") + ex.what(), {});
      code = 1;
    }
    CoUninitialize();
    return code;
  }

  auralite::ui::Window window;
  if (!window.Create(title, 640, 720)) {
    MessageBoxW(nullptr, L"Window / Canvas init failed", L"ui_gallery",
                MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  InitGalleryThemes();

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
        write_dump_check(false, yaml_dump, fluent_dump);
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
  std::unique_ptr<auralite::ui::Window> modeless;
  GalleryState gallery_state;
  WireInteractive(root.get(), status, &window, &modeless, &gallery_state);

  window.set_accept_files(true);
  window.set_on_files_dropped([status, &window](const auralite::ui::FileDropEvent& e) {
    if (!status || e.paths.empty()) {
      return;
    }
    std::wstring msg = L"文件 " + std::to_wstring(e.paths.size()) + L": " +
                       e.paths.front();
    status->text(msg);
    window.Invalidate();
  });

  auralite::ui::SplitView* split_ptr = FindSplit(root.get());
  auralite::ui::PopupHost popup_host;
  const std::string popup_yaml = ResolvePopupMenuYaml();

  WireMenuStyleButtons(root.get(), &popup_host, &window, status, split_ptr);

  root->set_on_context_menu(
      [&popup_host, &window, &popup_yaml, status, split_ptr](int sx, int sy) {
        ShowMenuYaml(&popup_host, &window, status, split_ptr, popup_yaml,
                     POINT{sx, sy}, L"经典扁平");
      });

  if (status && !fluent) {
    status->text(L"YAML 模式 · 点菜单样式按钮或右键经典菜单");
  }

  window.SetRoot(std::move(root));
  window.FocusNext(false);

  ShowWindow(window.hwnd(), show);
  UpdateWindow(window.hwnd());

  const int code = auralite::ui::Application::Run();
  CoUninitialize();
  return code;
}
