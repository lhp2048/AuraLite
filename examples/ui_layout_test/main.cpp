// Console layout smoke tests for Column/Row weight and Absolute anchors.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "auralite/ui/absolute.h"
#include "auralite/ui/button.h"
#include "auralite/ui/column.h"
#include "auralite/ui/factory.h"
#include "auralite/ui/label.h"
#include "auralite/ui/native_host.h"
#include "auralite/ui/node.h"
#include "auralite/ui/row.h"
#include "auralite/ui/theme.h"
#include "auralite/ui/tile.h"
#include "auralite/ui/text_layout.h"
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

void TestColumnHugFillChildren() {
  using namespace auralite::ui;
  Column col;
  col.hug_width();
  col.padding(0.f);
  col.spacing(0.f);

  auto btn = std::make_unique<Button>();
  btn->text(L"OK");
  btn->fill_width();
  btn->set_preferred_width(0.f);
  btn->fixed_height(32.f);
  col.AddChild(std::move(btn));

  const SizeF s = col.Measure(400.f, 800.f);
  Expect("hug col narrower than measure cap", s.w < 200.f);
  Expect("hug col wider than empty", s.w > 24.f);
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
  Expect("dialog resizable off", !spec.options.resizable);
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

  WindowYaml framed;
  auto framed_n = LoadYamlString(
      "window:\n"
      "  caption: false\n"
      "  resizable: true\n"
      "  min_width: 200\n"
      "  min_height: 100\n"
      "Column:\n"
      "  children: []\n",
      factory, {}, &framed);
  Expect("framed yaml", framed_n != nullptr);

  WindowYaml themed;
  auto themed_n = LoadYamlString(
      "window:\n"
      "  kind: dialog\n"
      "  theme: dark\n"
      "Column:\n"
      "  children: []\n",
      factory, {}, &themed);
  Expect("themed yaml tree", themed_n != nullptr);
  Expect("window theme dark", themed.theme == "dark");
  Expect("framed no theme", framed.theme.empty());
  Expect("caption false resizable", framed.options.resizable);
  Expect("min_width", framed.options.min_width == 200);
  Expect("min_height", framed.options.min_height == 100);
}

void TestResizeHitTest() {
  using namespace auralite::ui;
  Expect("center is not edge",
         Window::HitTestResizeEdge(50.f, 50.f, 100.f, 100.f, 6.f, 12.f) ==
             HTNOWHERE);
  Expect("left edge",
         Window::HitTestResizeEdge(2.f, 50.f, 100.f, 100.f, 6.f, 12.f) ==
             HTLEFT);
  Expect("right edge",
         Window::HitTestResizeEdge(98.f, 50.f, 100.f, 100.f, 6.f, 12.f) ==
             HTRIGHT);
  Expect("top edge",
         Window::HitTestResizeEdge(50.f, 2.f, 100.f, 100.f, 6.f, 12.f) ==
             HTTOP);
  Expect("bottom edge",
         Window::HitTestResizeEdge(50.f, 98.f, 100.f, 100.f, 6.f, 12.f) ==
             HTBOTTOM);
  Expect("top-left corner",
         Window::HitTestResizeEdge(2.f, 2.f, 100.f, 100.f, 6.f, 12.f) ==
             HTTOPLEFT);
  Expect("bottom-right corner",
         Window::HitTestResizeEdge(98.f, 98.f, 100.f, 100.f, 6.f, 12.f) ==
             HTBOTTOMRIGHT);
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

  TitleBar stdbar;
  stdbar.title(L"Hello");
  stdbar.Layout(RectF{0.f, 0.f, 320.f, 36.f});
  Expect("default chrome title+min+max+close", stdbar.children().size() == 4);
  Expect("default title slot", stdbar.FindByName("title") != nullptr);
  Expect("default min slot", stdbar.FindByName("minimize") != nullptr);
  Expect("default max slot", stdbar.FindByName("maximize") != nullptr);
  Expect("default close slot", stdbar.FindByName("close") != nullptr);
  Expect("no icon without path", stdbar.FindByName("icon") == nullptr);
  Expect("title area drags", stdbar.HitTest(24.f, 18.f) == &stdbar);
  Node* close_hit = stdbar.HitTest(300.f, 18.f);
  Expect("close is a Button", dynamic_cast<Button*>(close_hit) != nullptr);

  TitleBar minbar;
  minbar.title(L"App").minimize(false);
  minbar.Layout(RectF{0.f, 0.f, 320.f, 36.f});
  Expect("minimize false drops min", minbar.children().size() == 3);
  Expect("min slot gone", minbar.FindByName("minimize") == nullptr);

  ViewFactory factory;
  auto custom = LoadYamlString(
      "TitleBar:\n"
      "  title: Ignored\n"
      "  close: true\n"
      "  children:\n"
      "    - Label: { text: T }\n",
      factory, {});
  Expect("yaml TitleBar", dynamic_cast<TitleBar*>(custom.get()) != nullptr);
  custom->Layout(RectF{0.f, 0.f, 200.f, 36.f});
  Expect("children list is the layout", custom->children().size() == 1);

  auto yaml_host = LoadYamlString(
      "NativeHost:\n"
      "  width: fill\n"
      "  height: 80\n",
      factory, {});
  Expect("yaml NativeHost",
         dynamic_cast<NativeHost*>(yaml_host.get()) != nullptr);

  auto yaml_std = LoadYamlString(
      "TitleBar:\n"
      "  title: Hello\n",
      factory, {});
  yaml_std->Layout(RectF{0.f, 0.f, 320.f, 36.f});
  Expect("yaml default chrome 4 slots", yaml_std->children().size() == 4);

  auto yaml_no_min = LoadYamlString(
      "TitleBar:\n"
      "  title: Hello\n"
      "  minimize: false\n",
      factory, {});
  yaml_no_min->Layout(RectF{0.f, 0.f, 320.f, 36.f});
  Expect("yaml minimize false", yaml_no_min->children().size() == 3);

  auto only_new = LoadYamlString(
      "TitleBar:\n"
      "  title: Hello\n"
      "  close: true\n"
      "  children:\n"
      "    - Button: { text: \"?\" }\n",
      factory, {});
  only_new->Layout(RectF{0.f, 0.f, 200.f, 36.f});
  Expect("one extra button is the whole bar", only_new->children().size() == 1);
  auto* only_btn = dynamic_cast<Button*>(only_new->children()[0].get());
  Expect("extra button keeps text", only_btn && only_btn->text() == L"?");

  auto only_close = LoadYamlString(
      "TitleBar:\n"
      "  children:\n"
      "    - Button: { name: close }\n",
      factory, {});
  only_close->Layout(RectF{0.f, 0.f, 200.f, 36.f});
  Expect("named close only", only_close->children().size() == 1);
  auto* close_only = dynamic_cast<Button*>(only_close->FindByName("close"));
  Expect("close slot fills glyph", close_only && close_only->text() == L"\u00D7");
  Expect("close slot has click", close_only && close_only->has_on_click());

  auto overlay = LoadYamlString(
      "TitleBar:\n"
      "  title: App\n"
      "  children:\n"
      "    - Label: { name: title }\n"
      "    - Button: { text: \"?\" }\n"
      "    - Button: { name: close, text: X }\n",
      factory, {});
  overlay->Layout(RectF{0.f, 0.f, 320.f, 36.f});
  Expect("declared slots plus extra", overlay->children().size() == 3);
  auto* title_lab = dynamic_cast<Label*>(overlay->FindByName("title"));
  Expect("title slot uses TitleBar.title", title_lab && title_lab->text() == L"App");
  auto* close_ov = dynamic_cast<Button*>(overlay->FindByName("close"));
  Expect("close text overlays", close_ov && close_ov->text() == L"X");
}

void TestLabelText() {
  using namespace auralite::ui;
  Theme::RegisterBuiltInLight();
  const wchar_t* font = Theme::Active().font_ui.c_str();
  const std::wstring sample = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  Expect("clip returns full text",
         EllipsizeUiText(sample, 10000.f, 16.f, font, TextTrim::Clip) == sample);
  Expect("wide end keeps full",
         EllipsizeUiText(sample, 10000.f, 16.f, font, TextTrim::End) == sample);

  const std::wstring end =
      EllipsizeUiText(sample, 48.f, 16.f, font, TextTrim::End);
  Expect("end has ellipsis",
         !end.empty() && end.back() == kEllipsis && end.size() < sample.size());

  const std::wstring start =
      EllipsizeUiText(sample, 48.f, 16.f, font, TextTrim::Start);
  Expect("start has ellipsis",
         !start.empty() && start.front() == kEllipsis &&
             start.size() < sample.size());

  const std::wstring mid =
      EllipsizeUiText(sample, 64.f, 16.f, font, TextTrim::Middle);
  Expect("middle has ellipsis", mid.find(kEllipsis) != std::wstring::npos);
  Expect("middle not prefix-only",
         mid.size() >= 2 && mid.front() != kEllipsis && mid.back() != kEllipsis);

  std::vector<std::wstring> hard;
  WrapUiText(L"one\ntwo", 10000.f, 16.f, font, &hard);
  Expect("hard break two lines", hard.size() == 2);
  Expect("hard first", hard[0] == L"one");
  Expect("hard second", hard[1] == L"two");

  Label wrapped;
  wrapped.text(sample).font_size(16.f).wrap(true);
  const SizeF wide = wrapped.Measure(10000.f, 10000.f);
  const SizeF narrow = wrapped.Measure(40.f, 10000.f);
  Expect("wrap grows height", narrow.h > wide.h + 8.f);

  Label single;
  single.text(sample).font_size(16.f);
  const SizeF one = single.Measure(40.f, 10000.f);
  Expect("single line height", one.h < narrow.h);

  ViewFactory factory;
  auto n = LoadYamlString(
      "Label:\n  text: Hello\n  wrap: true\n  trim: middle\n", factory, {});
  auto* lab = dynamic_cast<Label*>(n.get());
  Expect("yaml wrap", lab && lab->wrap());
  Expect("yaml trim middle", lab && lab->trim() == TextTrim::Middle);
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
  TestColumnHugFillChildren();
  TestClipDefaults();
  TestClipYaml();
  TestHitTestStillClipsToBounds();
  TestTooltipResolve();
  TestWindowYaml();
  TestResizeHitTest();
  TestTitleBarHitTest();
  TestLabelText();
  if (g_failures > 0) {
    std::printf("%d failure(s)\n", g_failures);
    return 1;
  }
  std::printf("all layout tests passed\n");
  return 0;
}
