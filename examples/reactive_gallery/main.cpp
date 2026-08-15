#include <windows.h>
#include <objbase.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "auralite/async/awaiters.h"
#include "auralite/reactive/observe.h"
#include "auralite/reactive/signal.h"
#include "auralite/ui/absolute.h"
#include "auralite/ui/application.h"
#include "auralite/ui/bind.h"
#include "auralite/ui/button.h"
#include "auralite/ui/checkbox.h"
#include "auralite/ui/column.h"
#include "auralite/ui/combo.h"
#include "auralite/ui/context_menu.h"
#include "auralite/ui/image_button.h"
#include "auralite/ui/image_view.h"
#include "auralite/ui/label.h"
#include "auralite/ui/progress_bar.h"
#include "auralite/ui/radio.h"
#include "auralite/ui/row.h"
#include "auralite/ui/scroll_view.h"
#include "auralite/ui/slider.h"
#include "auralite/ui/split_view.h"
#include "auralite/ui/switch_control.h"
#include "auralite/ui/tab.h"
#include "auralite/ui/text_area.h"
#include "auralite/ui/text_field.h"
#include "auralite/ui/theme.h"
#include "auralite/ui/tile.h"
#include "auralite/ui/tree_view.h"
#include "auralite/ui/virtual_list.h"
#include "auralite/ui/window.h"

namespace {

using auralite::async::Delay;
using auralite::async::FireAndForget;
using auralite::async::RunAsync;
using auralite::async::SpawnUi;
using auralite::reactive::Computed;
using auralite::reactive::Signal;
using namespace auralite::ui;

struct GalleryModel {
  Signal<std::wstring> status{L"Ready — Phase 3 reactive gallery"};
  Signal<std::wstring> name{L""};
  Signal<bool> remember{true};
  Signal<bool> notify_on{true};
  Signal<bool> show_extra{true};
  Signal<float> progress{0.35f};
  Signal<bool> progress_busy{false};
  Signal<bool> loading{false};
  Signal<std::vector<std::wstring>> list_items;
  Computed<std::wstring> form_summary{[this] {
    return L"摘要: name=\"" + name.Get() + L"\"  remember=" +
           (remember.Get() ? L"on" : L"off") + L"  notify=" +
           (notify_on.Get() ? L"on" : L"off");
  }};
  Computed<bool> can_load{[this] { return !loading.Get(); }};
};

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

void InitThemes() {
  Theme::RegisterBuiltInLight();
  Theme::RegisterBuiltInDark();
  namespace fs = std::filesystem;
  const std::wstring beside = ExeDir() + L"\\themes";
  if (fs::is_directory(beside)) {
    Theme::RegisterFromDir(NarrowPath(beside));
  } else if (fs::is_directory("examples/ui_gallery/themes")) {
    Theme::RegisterFromDir("examples/ui_gallery/themes");
  }
  Theme::SetActive("light");
}

template <typename T, typename... Args>
T* Add(Node* parent, Args&&... args) {
  auto node = std::make_unique<T>(std::forward<Args>(args)...);
  T* raw = node.get();
  parent->AddChild(std::move(node));
  return raw;
}

FireAndForget LoadListAsync(std::shared_ptr<GalleryModel> model,
                            std::shared_ptr<std::atomic_bool> alive) {
  model->loading.Set(true);
  model->status.Set(L"Loading list…");
  co_await Delay(40, alive);
  co_await RunAsync(
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(120)); },
      alive);
  if (!alive || !alive->load() || !model) {
    co_return;
  }
  std::vector<std::wstring> rows;
  rows.reserve(30);
  for (int i = 1; i <= 30; ++i) {
    rows.push_back(L"Reactive item #" + std::to_wstring(i));
  }
  model->list_items.Set(std::move(rows));
  model->loading.Set(false);
  model->status.Set(L"List loaded (30 rows)");
}

FireAndForget SimulateProgressAsync(std::shared_ptr<GalleryModel> model,
                                    std::shared_ptr<std::atomic_bool> alive) {
  model->loading.Set(true);
  model->progress_busy.Set(true);
  model->status.Set(L"Async progress…");
  co_await Delay(30, alive);
  co_await RunAsync(
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(200)); },
      alive);
  if (!alive || !alive->load() || !model) {
    co_return;
  }
  model->progress_busy.Set(false);
  model->progress.Set(1.f);
  model->loading.Set(false);
  model->status.Set(L"Async progress done");
}

std::unique_ptr<Node> BuildFormPage(const std::shared_ptr<GalleryModel>& model) {
  auto page = std::make_unique<ScrollView>();
  page->fill_width().fill_height();
  auto col = std::make_unique<Column>();
  Column* c = col.get();
  c->padding(16.f).spacing(10.f);

  Add<Label>(c)->text(L"表单").font_size(18.f);
  Add<Label>(c)->text(L"TextField / Check / Radio / Switch + Bind").font_size(13.f);

  auto* summary = Add<Label>(c);
  summary->fixed_height(28.f);
  c->OwnSubscription(BindText(*summary, model->form_summary));

  Add<Label>(c)->text(L"Name").font_size(13.f).fixed_height(18.f);
  auto* field = Add<TextField>(c);
  field->placeholder(L"输入名字（IME OK）");
  field->on_change([model](const std::wstring& t) { model->name.Set(t); });

  auto* pass = Add<TextField>(c);
  pass->placeholder(L"密码（不回显）").password(true);

  auto* row = Add<Row>(c);
  row->spacing(16.f);
  auto* remember = Add<Checkbox>(row);
  remember->text(L"记住选项");
  remember->on_changed([model](bool v) { model->remember.Set(v); });
  row->OwnSubscription(BindChecked(*remember, model->remember));
  auto* notify = Add<Switch>(row);
  notify->text(L"通知").on(true);
  notify->on_changed([model](bool v) {
    model->notify_on.Set(v);
    model->show_extra.Set(v);
  });

  auto* radios = Add<Row>(c);
  radios->spacing(16.f);
  auto* ra = Add<Radio>(radios);
  ra->text(L"选项 A").group_id(1).checked(true);
  ra->on_changed([model](bool on) {
    if (on) {
      model->status.Set(L"Radio → A");
    }
  });
  auto* rb = Add<Radio>(radios);
  rb->text(L"选项 B").group_id(1);
  rb->on_changed([model](bool on) {
    if (on) {
      model->status.Set(L"Radio → B");
    }
  });

  auto* extra = Add<Label>(c);
  extra->text(L"额外提示：关闭「通知」会隐藏本行（BindVisible）");
  extra->fixed_height(36.f);
  c->OwnSubscription(BindVisible(*extra, model->show_extra));

  auto* btn_row = Add<Row>(c);
  btn_row->spacing(8.f);
  auto* go = Add<Button>(btn_row);
  go->text(L"提交").hug_width().fixed_height(36.f);
  go->on_click([model] {
    model->status.Set(L"Submitted: " + model->name.Peek());
  });

  page->set_content(std::move(col));
  return page;
}

std::unique_ptr<Node> BuildFeedbackPage(Window* window,
                                        const std::shared_ptr<GalleryModel>& model) {
  auto page = std::make_unique<ScrollView>();
  page->fill_width().fill_height();
  auto col = std::make_unique<Column>();
  Column* c = col.get();
  c->padding(16.f).spacing(10.f);

  Add<Label>(c)->text(L"反馈").font_size(18.f);
  Add<Label>(c)->text(L"Slider ↔ Progress · Combo · TextArea · 异步加载")
      .font_size(13.f);

  auto* prog = Add<ProgressBar>(c);
  prog->BindWindow(window);
  prog->value(model->progress.Peek()).fixed_height(16.f).fill_width();
  c->OwnSubscription(BindValue(*prog, model->progress));
  c->OwnSubscription(BindIndeterminate(*prog, model->progress_busy));

  auto* slider = Add<Slider>(c);
  slider->value(model->progress.Peek()).tick_count(5).fill_width().fixed_height(28.f);
  c->OwnSubscription(BindValue(*slider, model->progress));
  slider->on_changed([model](float v) {
    if (model->progress_busy.Peek()) {
      return;
    }
    model->progress.Set(v);
    model->status.Set(L"Progress " + std::to_wstring(static_cast<int>(v * 100)) +
                      L"%");
  });

  auto* combo = Add<Combo>(c);
  combo->BindWindow(window);
  combo->items({L"苹果", L"香蕉", L"橙子", L"葡萄"}).selected(0).fill_width();
  combo->on_changed([model](int i) {
    model->status.Set(L"Combo index " + std::to_wstring(i));
  });

  auto* area = Add<TextArea>(c);
  area->set_text(L"多行文本（TextArea）\n第二行");
  area->fixed_height(100.f).fill_width();

  auto* load = Add<Button>(c);
  load->text(L"模拟异步进度");
  c->OwnSubscription(BindEnabled(*load, model->can_load));
  load->on_click([model, alive = window->alive_flag()] {
    SpawnUi(SimulateProgressAsync(model, alive));
  });

  page->set_content(std::move(col));
  return page;
}

std::unique_ptr<Node> BuildListPage(Window* window,
                                    const std::shared_ptr<GalleryModel>& model) {
  auto page = std::make_unique<Column>();
  Column* c = page.get();
  c->padding(16.f).spacing(10.f).fill_width().fill_height();

  Add<Label>(c)->text(L"列表 / 树").font_size(18.f);

  auto* tools = Add<Row>(c);
  tools->spacing(8.f);
  auto* load = Add<Button>(tools);
  load->text(L"Load VirtualList").hug_width().fixed_height(32.f);
  load->on_click([model, alive = window->alive_flag()] {
    SpawnUi(LoadListAsync(model, alive));
  });
  auto* clear = Add<Button>(tools);
  clear->text(L"Clear").hug_width().fixed_height(32.f);
  clear->on_click([model] {
    model->list_items.Set({});
    model->status.Set(L"List cleared");
  });

  auto* list = Add<VirtualList>(c);
  list->fill_width().fill_height();
  list->row_height(VirtualListItemKind::Text, 32.f);
  c->OwnSubscription(BindItems(*list, model->list_items));

  auto* tree = Add<TreeView>(c);
  tree->fixed_height(160.f).fill_width();
  const int root = tree->AddRoot(L"Reactive tree", true);
  const int c1 = tree->AddChild(root, L"Child A");
  tree->AddChild(c1, L"Leaf 1");
  tree->AddChild(root, L"Child B");

  return page;
}

std::unique_ptr<Node> BuildLayoutPage(Window* window,
                                     const std::shared_ptr<GalleryModel>& model) {
  auto page = std::make_unique<ScrollView>();
  page->fill_width().fill_height();
  auto col = std::make_unique<Column>();
  Column* c = col.get();
  c->padding(16.f).spacing(10.f);

  Add<Label>(c)->text(L"布局").font_size(18.f);

  auto* split = Add<SplitView>(c);
  split->set_ratio(0.45f);
  split->fill_width().fixed_height(90.f);
  {
    auto left = std::make_unique<Label>();
    left->text(L"Split 左").align(TextAlign::Center);
    auto right = std::make_unique<Label>();
    right->text(L"Split 右").align(TextAlign::Center);
    split->set_leading(std::move(left));
    split->set_trailing(std::move(right));
  }

  Add<Label>(c)->text(L"Tile").font_size(13.f).fixed_height(18.f);
  auto* tile = Add<Tile>(c);
  tile->columns(3).item_size(80.f, 36.f).spacing(8.f);
  for (int i = 1; i <= 6; ++i) {
    auto* b = Add<Button>(tile);
    b->text(L"T" + std::to_wstring(i)).preferred_size(80.f, 36.f);
    b->on_click([model, i] {
      model->status.Set(L"Tile T" + std::to_wstring(i));
    });
  }

  Add<Label>(c)->text(L"Absolute").font_size(13.f).fixed_height(18.f);
  auto* abs = Add<Absolute>(c);
  abs->fixed_height(80.f).fill_width();
  auto* floating = Add<Label>(abs);
  floating->text(L"pos(12,20)").font_size(13.f);
  floating->hug_width().hug_height().set_pos(12.f, 20.f);

  auto* imgs = Add<Row>(c);
  imgs->spacing(12.f);
  auto* image = Add<ImageView>(imgs);
  {
    const auto checker = MakeCheckerBgra(64, 8);
    image->SetPixels(64, 64, checker.data(), 64 * 4).preferred_size(64.f, 64.f);
  }
  auto* ib = Add<ImageButton>(imgs);
  {
    const auto solid = MakeSolidBgra(32, 60, 140, 220);
    ib->SetPixels(32, 32, solid.data(), 32 * 4).preferred_size(48.f, 48.f);
  }
  ib->on_click([model] { model->status.Set(L"ImageButton clicked"); });
  Add<Label>(c)
      ->text(L"左 ImageView（棋盘） · 右 ImageButton（纯色，可点）")
      .font_size(13.f)
      .fixed_height(22.f);

  static ContextMenu s_menu;
  static bool menu_inited = false;
  if (!menu_inited) {
    s_menu.AddItem(1, L"刷新状态").AddSeparator().AddItem(2, L"关于");
    menu_inited = true;
  }
  s_menu.on_command([model](int id) {
    model->status.Set(L"ContextMenu id=" + std::to_wstring(id));
  });
  c->set_on_context_menu([window](int sx, int sy) {
    s_menu.Show(window->hwnd(), sx, sy);
  });
  Add<Label>(c)
      ->text(L"在本页空白处右键 → ContextMenu")
      .font_size(13.f)
      .fixed_height(24.f);

  page->set_content(std::move(col));
  return page;
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int show) {
  Application::EnableDpiAwareness();
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  InitThemes();

  Window window;
  if (!window.Create(L"AuraLite reactive_gallery", 720, 780)) {
    MessageBoxW(nullptr, L"Window init failed", L"reactive_gallery", MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  auto model = std::make_shared<GalleryModel>();
  auto alive = window.alive_flag();

  auto root = std::make_unique<Column>();
  Column* root_c = root.get();
  root_c->padding(12.f).spacing(8.f).fill_width().fill_height();

  Add<Label>(root_c)->text(L"AuraLite Reactive Gallery").font_size(22.f);

  auto* status = Add<Label>(root_c);
  status->fixed_height(26.f);
  root_c->OwnSubscription(BindText(*status, model->status));

  auto* themes = Add<Row>(root_c);
  themes->spacing(8.f);
  auto* light = Add<Button>(themes);
  light->text(L"Light").hug_width().fixed_height(32.f);
  light->on_click([model] {
    Theme::SetActive("light");
    model->status.Set(L"Theme → light");
  });
  auto* dark = Add<Button>(themes);
  dark->text(L"Dark").hug_width().fixed_height(32.f);
  dark->on_click([model] {
    Theme::SetActive("dark");
    model->status.Set(L"Theme → dark");
  });

  auto* tab = Add<Tab>(root_c);
  tab->set_headers({L"表单", L"反馈", L"列表", L"布局"});
  tab->header_height(36.f);
  tab->set_selected(0);
  tab->fill_width().fill_height();
  tab->AddChild(BuildFormPage(model));
  tab->AddChild(BuildFeedbackPage(&window, model));
  tab->AddChild(BuildListPage(&window, model));
  tab->AddChild(BuildLayoutPage(&window, model));
  tab->on_selected([model](int i) {
    model->status.Set(L"Tab page " + std::to_wstring(i));
  });

  window.SetRoot(std::move(root));
  ShowWindow(window.hwnd(), show);
  UpdateWindow(window.hwnd());
  const int code = Application::Run();
  CoUninitialize();
  return code;
}
