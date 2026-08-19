#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

#include "mx/ui/column.h"
#include "mx/ui/factory.h"
#include "mx/ui/label.h"
#include "mx/ui/progress_bar.h"
#include "mx/ui/scroll_view.h"
#include "mx/ui/slider.h"
#include "mx/ui/switch_control.h"
#include "mx/ui/tab.h"
#include "mx/ui/theme.h"
#include "mx/ui/window.h"
#include "mx/ui/yaml_loader.h"

namespace {

int g_failures = 0;

void Expect(const char* name, bool cond) {
  if (!cond) {
    std::printf("FAIL %s\n", name);
    ++g_failures;
  } else {
    std::printf("ok   %s\n", name);
  }
}

void ExpectNear(const char* name, float got, float want, float eps = 0.001f) {
  if (got < want - eps || got > want + eps) {
    std::printf("FAIL %s: got=%.4f want=%.4f\n", name, got, want);
    ++g_failures;
  } else {
    std::printf("ok   %s\n", name);
  }
}

}  // namespace

int main() {
  setvbuf(stdout, nullptr, _IONBF, 0);
  using namespace mx::ui;
  Theme::RegisterBuiltInLight();
  Theme::SetActive("light");

  {
    Switch sw;
    Expect("node animate default", sw.animate());
    sw.animate(false);
    Expect("node animate off", !sw.animate());
    sw.animate(true);
    Expect("node animate on", sw.animate());
  }

  {
    ViewFactory factory;
    auto n = LoadYamlString("Switch:\n  text: x\n  animate: false\n", factory,
                            {});
    auto* sw = dynamic_cast<Switch*>(n.get());
    Expect("yaml switch", sw != nullptr);
    Expect("yaml animate false", sw && !sw->animate());
  }

  {
    Switch sw;
    sw.on(true);
    ExpectNear("switch snap no hwnd", sw.thumb_t(), 1.f);
    sw.animate(false);
    sw.on(false);
    ExpectNear("switch off snap", sw.thumb_t(), 0.f);
  }

  {
    Slider sl;
    sl.animate(false);
    sl.value(0.4f);
    ExpectNear("slider snap", sl.visual_value(), 0.4f);
  }

  {
    ProgressBar bar;
    bar.animate(false);
    bar.value(0.7f);
    ExpectNear("progress snap", bar.visual_value(), 0.7f);
  }

  {
    Tab tab;
    tab.set_headers({L"A", L"B", L"C"});
    tab.AddChild(std::make_unique<Label>());
    tab.AddChild(std::make_unique<Label>());
    tab.AddChild(std::make_unique<Label>());
    tab.set_selected(2);
    ExpectNear("tab snap", tab.visual_selected(), 2.f);
  }

  {
    auto sv = std::make_unique<ScrollView>();
    auto col = std::make_unique<Column>();
    col->fixed_height(800.f);
    sv->preferred_size(120.f, 80.f);
    sv->set_content(std::move(col));
    sv->Layout(RectF{0.f, 0.f, 120.f, 80.f});
    sv->animate(false);
    sv->set_scroll_offset(40.f);
    ExpectNear("scroll snap", sv->scroll_offset(), 40.f);
  }

  {
    Window w;
    Window::WindowOptions opt;
    opt.quit_on_close = false;
    if (!w.Create(L"control_anim_test", 320, 240, opt)) {
      Expect("Create", false);
    } else {
      auto sw = std::make_unique<Switch>();
      Switch* p = sw.get();
      w.SetRoot(std::move(sw));
      p->on(true);
      Expect("switch anim pending", p->is_on() && p->thumb_t() < 1.f);
      p->animate(false);
      ExpectNear("switch anim cancel snaps", p->thumb_t(), 1.f);
    }
  }

  if (g_failures) {
    std::printf("%d failed\n", g_failures);
    return EXIT_FAILURE;
  }
  std::printf("all ok\n");
  return EXIT_SUCCESS;
}
