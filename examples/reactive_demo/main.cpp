#include <windows.h>
#include <objbase.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "auralite/async/awaiters.h"
#include "auralite/reactive/signal.h"
#include "auralite/ui/application.h"
#include "auralite/ui/bind.h"
#include "auralite/ui/button.h"
#include "auralite/ui/column.h"
#include "auralite/ui/label.h"
#include "auralite/ui/virtual_list.h"
#include "auralite/ui/window.h"

namespace {

using auralite::async::Delay;
using auralite::async::RunAsync;
using auralite::async::SpawnUi;
using auralite::reactive::Computed;
using auralite::reactive::Signal;
using auralite::ui::BindEnabled;
using auralite::ui::BindItems;
using auralite::ui::BindText;
using auralite::ui::BindVisible;
using auralite::ui::Button;
using auralite::ui::Column;
using auralite::ui::Label;
using auralite::ui::VirtualList;
using auralite::ui::Window;

struct DemoModel {
  Signal<int> count{0};
  Signal<bool> show_hint{true};
  Signal<bool> load_enabled{true};
  Signal<std::vector<std::wstring>> items;
  Computed<std::wstring> count_text{[this] {
    return L"Count: " + std::to_wstring(count.Get());
  }};
};

// Free-function coroutine (MSVC is fragile with temporary coroutine lambdas).
auralite::async::FireAndForget LoadItemsAsync(
    std::shared_ptr<DemoModel> model,
    std::shared_ptr<std::atomic_bool> alive) {
  co_await Delay(50, alive);
  co_await RunAsync(
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(80)); },
      alive);
  if (!alive || !alive->load() || !model) {
    co_return;
  }
  std::vector<std::wstring> rows;
  rows.reserve(20);
  for (int i = 0; i < 20; ++i) {
    rows.push_back(L"Item #" + std::to_wstring(i + 1));
  }
  model->items.Set(std::move(rows));
  model->load_enabled.Set(true);
  model->count.Set(model->count.Peek() + 1);
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int show) {
  auralite::ui::Application::EnableDpiAwareness();
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  Window window;
  if (!window.Create(L"AuraLite reactive_demo", 480, 640)) {
    MessageBoxW(nullptr, L"Window / Canvas init failed", L"reactive_demo",
                MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  auto alive = window.alive_flag();
  auto model = std::make_shared<DemoModel>();

  auto root = std::make_unique<Column>();
  Column* column = root.get();
  column->padding(20.f).spacing(12.f);

  auto* title = new Label();
  title->text(L"Phase 3 — Signal + async").font_size(18.f);
  column->AddChild(std::unique_ptr<Label>(title));

  auto* count_label = new Label();
  count_label->fixed_height(28.f);
  column->AddChild(std::unique_ptr<Label>(count_label));
  column->OwnSubscription(BindText(*count_label, model->count_text));

  auto* hint = new Label();
  hint->text(L"Hint line — Toggle hint hides/shows this text.");
  hint->fixed_height(40.f);
  column->AddChild(std::unique_ptr<Label>(hint));
  column->OwnSubscription(BindVisible(*hint, model->show_hint));

  auto* inc = new Button();
  inc->text(L"Increment").on_click([model] {
    model->count.Set(model->count.Peek() + 1);
  });
  column->AddChild(std::unique_ptr<Button>(inc));

  auto* toggle = new Button();
  toggle->text(L"Toggle hint").on_click([model] {
    model->show_hint.Set(!model->show_hint.Peek());
  });
  column->AddChild(std::unique_ptr<Button>(toggle));

  auto* load = new Button();
  load->text(L"Load items");
  column->OwnSubscription(BindEnabled(*load, model->load_enabled));
  load->on_click([model, alive] {
    model->load_enabled.Set(false);
    SpawnUi(LoadItemsAsync(model, alive));
  });
  column->AddChild(std::unique_ptr<Button>(load));

  auto* list = new VirtualList();
  list->fill_width().fill_height();
  list->row_height(auralite::ui::VirtualListItemKind::Text, 32.f);
  column->AddChild(std::unique_ptr<VirtualList>(list));
  column->OwnSubscription(BindItems(*list, model->items));

  window.SetRoot(std::move(root));
  ShowWindow(window.hwnd(), show);
  UpdateWindow(window.hwnd());
  const int code = auralite::ui::Application::Run();
  CoUninitialize();
  return code;
}
