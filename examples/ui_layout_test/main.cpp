// Console layout smoke tests for Column/Row weight and Absolute anchors.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

#include "auralite/ui/absolute.h"
#include "auralite/ui/button.h"
#include "auralite/ui/column.h"
#include "auralite/ui/factory.h"
#include "auralite/ui/label.h"
#include "auralite/ui/node.h"
#include "auralite/ui/row.h"
#include "auralite/ui/theme.h"
#include "auralite/ui/tile.h"
#include "auralite/ui/title_bar.h"
#include "auralite/ui/yaml_loader.h"

namespace {

int g_failures = 0;

void ExpectNear(const char* name, float got, float want, float eps = 0.5f) {
  if (std::fabs(got - want) > eps) {
    std::printf("FAIL %s: got=%.2f want=%.2f\n", name, got, want);
    ++g_failures;
  } else {
    std::printf("ok   %s\n", name);
  }
}

void Expect(const char* name, bool cond) {
  if (!cond) {
    std::printf("FAIL %s\n", name);
    ++g_failures;
  } else {
    std::printf("ok   %s\n", name);
  }
}

bool ColorEq(const auralite::ColorF& a, const auralite::ColorF& b,
             float eps = 0.01f) {
  return std::fabs(a.r - b.r) < eps && std::fabs(a.g - b.g) < eps &&
         std::fabs(a.b - b.b) < eps && std::fabs(a.a - b.a) < eps;
}

void TestRowWeight() {
  using namespace auralite::ui;
  auto row = std::make_unique<Row>();
  row->spacing(0.f);
  row->fixed_height(40.f);

  auto a = std::make_unique<Button>();
  a->text(L"a");
  a->fill_width();
  a->fixed_height(32.f);
  a->weight(1.f);

  auto b = std::make_unique<Button>();
  b->text(L"b");
  b->fill_width();
  b->fixed_height(32.f);
  b->weight(3.f);

  Button* pa = a.get();
  Button* pb = b.get();
  row->AddChild(std::move(a));
  row->AddChild(std::move(b));
  row->Layout(RectF{0, 0, 400, 40});

  ExpectNear("row weight a.w", pa->bounds().w, 100.f);
  ExpectNear("row weight b.w", pb->bounds().w, 300.f);
}

void TestWeightIgnoredWithoutFill() {
  using namespace auralite::ui;
  auto row = std::make_unique<Row>();
  row->spacing(0.f);
  row->fixed_height(40.f);

  auto a = std::make_unique<Button>();
  a->text(L"a");
  a->hug_width();
  a->fixed_width(80.f);
  a->fixed_height(28.f);
  a->weight(99.f);  // ignored: not Fill

  auto b = std::make_unique<Button>();
  b->text(L"b");
  b->hug_width();
  b->fixed_width(80.f);
  b->fixed_height(28.f);

  Button* pa = a.get();
  row->AddChild(std::move(a));
  row->AddChild(std::move(b));
  row->Layout(RectF{0, 0, 400, 40});

  ExpectNear("fixed+weight keeps width", pa->bounds().w, 80.f);
}

void TestAbsoluteDualAnchor() {
  using namespace auralite::ui;
  auto host = std::make_unique<Absolute>();
  auto btn = std::make_unique<Button>();
  btn->text(L"x");
  btn->left(10.f);
  btn->right(10.f);
  btn->top(5.f);
  btn->fixed_height(30.f);
  Button* p = btn.get();
  host->AddChild(std::move(btn));
  host->Layout(RectF{0, 0, 200, 100});

  ExpectNear("abs left", p->bounds().x, 10.f);
  ExpectNear("abs dual w", p->bounds().w, 180.f);
  ExpectNear("abs top", p->bounds().y, 5.f);
  ExpectNear("abs h", p->bounds().h, 30.f);
}

void TestAbsoluteBottomRight() {
  using namespace auralite::ui;
  auto host = std::make_unique<Absolute>();
  auto btn = std::make_unique<Button>();
  btn->text(L"rb");
  btn->right(8.f);
  btn->bottom(8.f);
  btn->fixed_width(50.f);
  btn->fixed_height(20.f);
  Button* p = btn.get();
  host->AddChild(std::move(btn));
  host->Layout(RectF{0, 0, 200, 100});

  ExpectNear("abs rb x", p->bounds().x, 142.f);
  ExpectNear("abs rb y", p->bounds().y, 72.f);
}

void TestClipDefaults() {
  using namespace auralite::ui;
  Node n;
  Expect("Node clip default false", !n.clip_children());
  Absolute abs;
  Expect("Absolute clip default false", !abs.clip_children());
  Column col;
  Row row;
  Tile tile;
  Expect("Column clip default true", col.clip_children());
  Expect("Row clip default true", row.clip_children());
  Expect("Tile clip default true", tile.clip_children());
  col.clip_children(false);
  Expect("Column clip can disable", !col.clip_children());
}

void TestClipYaml() {
  using namespace auralite::ui;
  ViewFactory factory;
  auto a = LoadYamlString("Column:\n  children: []\n", factory, {});
  Expect("yaml Column clip default true", a && a->clip_children());
  auto b = LoadYamlString("Column:\n  clip: false\n  children: []\n", factory, {});
  Expect("yaml clip false", b && !b->clip_children());
  auto c = LoadYamlString("Absolute:\n  clip: true\n  children: []\n", factory, {});
  Expect("yaml Absolute clip true", c && c->clip_children());
}

void TestHitTestStillClipsToBounds() {
  using namespace auralite::ui;
  Column col;
  col.Layout(RectF{0, 0, 100, 40});
  auto child = std::make_unique<Button>();
  child->Layout(RectF{0, 0, 100, 80});
  Button* p = child.get();
  col.AddChild(std::move(child));
  Expect("hit inside parent", col.HitTest(10, 10) != nullptr);
  Expect("hit outside parent is miss", col.HitTest(10, 50) == nullptr);
  (void)p;
}

void TestTooltipResolve() {
  using namespace auralite::ui;
  Node parent;
  parent.tooltip(L"parent");
  auto child = std::make_unique<Node>();
  Node* pchild = child.get();
  parent.AddChild(std::move(child));
  Expect("child empty uses parent",
         ResolveTooltipText(pchild) && *ResolveTooltipText(pchild) == L"parent");
  pchild->tooltip(L"child");
  Expect("child wins",
         ResolveTooltipText(pchild) && *ResolveTooltipText(pchild) == L"child");
  Node bare;
  Expect("no tooltip is null", ResolveTooltipText(&bare) == nullptr);

  ViewFactory factory;
  auto n = LoadYamlString(
      "Button:\n  text: Save\n  tooltip: 保存\n", factory, {});
  Expect("yaml tooltip", n && n->tooltip() == L"保存");
}

void TestColumnMainAlignCenter() {
  using namespace auralite::ui;
  auto col = std::make_unique<Column>();
  col->spacing(0.f);
  col->v_align(Align::Center);

  auto a = std::make_unique<Button>();
  a->text(L"a");
  a->fixed_width(40.f);
  a->fixed_height(20.f);
  a->hug_width();
  Button* pa = a.get();
  col->AddChild(std::move(a));
  col->Layout(RectF{0, 0, 100, 100});

  ExpectNear("v_align center y", pa->bounds().y, 40.f);
}

}  // namespace

void TestWindowYaml() {
  using namespace auralite::ui;
  ViewFactory factory;
  WindowYaml spec;
  auto n = LoadYamlString(
      "window:\n"
      "  title: 对话框\n"
      "  width: 320\n"
      "  height: 180\n"
      "  kind: dialog\n"
      "  corner_radius: 12\n"
      "  border_width: 2\n"
      "  topmost: false\n"
      "Column:\n"
      "  children: []\n",
      factory, {}, &spec);
  Expect("window yaml tree is Column", n != nullptr);
  Expect("window present", spec.present);
  Expect("window title", spec.title == L"对话框");
  Expect("window width", spec.width == 320);
  Expect("window height", spec.height == 180);
  Expect("window kind dialog", spec.kind == WindowYaml::Kind::Dialog);
  Expect("window caption false", !spec.options.caption);
  Expect("window corner_radius", spec.options.corner_radius == 12.f);
  Expect("window border_width", spec.options.border_width == 2.f);
  Expect("window topmost false", !spec.options.topmost);
  Expect("create_options keeps radius",
         spec.create_options(nullptr).corner_radius == 12.f);
  Expect("window width_or", spec.width_or(100) == 320);

  WindowYaml main_spec;
  auto main_n = LoadYamlString(
      "window:\n"
      "  kind: main\n"
      "  corner_radius: 8\n"
      "  border_width: 1\n"
      "Column:\n"
      "  children: []\n",
      factory, {}, &main_spec);
  Expect("main yaml tree", main_n != nullptr);
  Expect("main kind", main_spec.kind == WindowYaml::Kind::Main);
  Expect("main still captioned", main_spec.options.caption);
  Expect("create_options drops radius on captioned",
         main_spec.create_options(nullptr).corner_radius == 0.f);
  Expect("create_options drops border on captioned",
         main_spec.create_options(nullptr).border_width == 0.f);

  WindowYaml square_spec;
  auto square_n = LoadYamlString(
      "window:\n"
      "  kind: dialog\n"
      "  corner_radius: 0\n"
      "Column:\n"
      "  children: []\n",
      factory, {}, &square_spec);
  Expect("square yaml tree", square_n != nullptr);
  Expect("square kind dialog", square_spec.kind == WindowYaml::Kind::Dialog);
  Expect("square caption false", !square_spec.options.caption);
  Expect("square radius 0", square_spec.create_options(nullptr).corner_radius == 0.f);

  WindowYaml missing;
  auto plain = LoadYamlString("Column:\n  children: []\n", factory, {},
                              &missing);
  Expect("plain yaml still loads", plain != nullptr);
  Expect("no window key", !missing.present);
}

void TestTitleBarHitTest() {
  using namespace auralite::ui;
  TitleBar bar;
  auto lab = std::make_unique<Label>();
  lab->text(L"Title").fill_width().fixed_height(36.f);
  auto btn = std::make_unique<Button>();
  btn->text(L"x").fixed_width(40.f).fixed_height(28.f);
  Button* pb = btn.get();
  bar.AddChild(std::move(lab));
  bar.AddChild(std::move(btn));
  bar.Layout(RectF{0.f, 0.f, 200.f, 36.f});
  Expect("label area is TitleBar (drag)", bar.HitTest(20.f, 18.f) == &bar);
  Expect("button keeps click", bar.HitTest(pb->bounds().x + 8.f, 18.f) == pb);

  ViewFactory factory;
  auto n = LoadYamlString(
      "TitleBar:\n"
      "  children:\n"
      "    - Label: { text: T }\n",
      factory, {});
  Expect("yaml TitleBar", dynamic_cast<TitleBar*>(n.get()) != nullptr);
}

void TestButtonVariant() {
  using namespace auralite::ui;
  Theme::RegisterBuiltInLight();
  Theme::RegisterBuiltInDark();
  Expect("set dark", Theme::SetActive("dark"));

  Button primary;
  Expect("default is Primary", primary.variant() == ButtonVariant::Primary);
  Expect("primary bg accent", ColorEq(primary.resolved_bg(), Theme::Active().accent));
  Expect("primary label on accent",
         ColorEq(primary.resolved_label(), Theme::Active().text_on_accent));

  Button secondary;
  secondary.variant(ButtonVariant::Secondary);
  Expect("secondary bg surface",
         ColorEq(secondary.resolved_bg(), Theme::Active().surface));
  Expect("secondary label text",
         ColorEq(secondary.resolved_label(), Theme::Active().text));
  secondary.OnMouseEnter(MouseEvent{});
  Expect("secondary hover accent_soft",
         ColorEq(secondary.resolved_bg(), Theme::Active().accent_soft));
  secondary.OnMouseLeave(MouseEvent{});

  Button danger;
  danger.variant(ButtonVariant::Danger);
  Expect("danger bg", ColorEq(danger.resolved_bg(), Theme::Active().danger));
  Expect("danger label on accent",
         ColorEq(danger.resolved_label(), Theme::Active().text_on_accent));

  const ColorF dark_surface = Theme::Active().surface;
  Expect("switch light", Theme::SetActive("light"));
  Expect("secondary follows new theme",
         ColorEq(secondary.resolved_bg(), Theme::Active().surface));
  Expect("secondary not frozen to dark",
         !ColorEq(secondary.resolved_bg(), dark_surface));

  Button frozen;
  frozen.bg(Theme::Active().danger);
  frozen.variant(ButtonVariant::Primary);
  Expect("variant clears bg override",
         ColorEq(frozen.resolved_bg(), Theme::Active().accent));

  Button custom;
  custom.variant(ButtonVariant::Secondary)
      .bg(auralite::ColorF::FromRgb(255, 0, 0));
  Expect("explicit bg still wins",
         ColorEq(custom.resolved_bg(), auralite::ColorF::FromRgb(255, 0, 0)));
  Expect("custom bg uses body text",
         ColorEq(custom.resolved_label(), Theme::Active().text));

  Button disabled;
  disabled.variant(ButtonVariant::Danger).set_enabled(false);
  Expect("disabled bg surface_alt",
         ColorEq(disabled.resolved_bg(), Theme::Active().surface_alt));
  Expect("disabled label muted",
         ColorEq(disabled.resolved_label(), Theme::Active().text_muted));

  ViewFactory factory;
  auto n = LoadYamlString(
      "Button:\n  text: Del\n  variant: danger\n", factory, {});
  auto* yaml_btn = dynamic_cast<Button*>(n.get());
  Expect("yaml danger variant",
         yaml_btn && yaml_btn->variant() == ButtonVariant::Danger);
  Expect("yaml danger bg",
         yaml_btn && ColorEq(yaml_btn->resolved_bg(), Theme::Active().danger));
}

int main() {
  TestButtonVariant();
  TestRowWeight();
  TestWeightIgnoredWithoutFill();
  TestAbsoluteDualAnchor();
  TestAbsoluteBottomRight();
  TestColumnMainAlignCenter();
  TestClipDefaults();
  TestClipYaml();
  TestHitTestStillClipsToBounds();
  TestTooltipResolve();
  TestWindowYaml();
  TestTitleBarHitTest();
  if (g_failures > 0) {
    std::printf("%d failure(s)\n", g_failures);
    return 1;
  }
  std::printf("all layout tests passed\n");
  return 0;
}
