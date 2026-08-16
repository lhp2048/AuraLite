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
#include "auralite/ui/node.h"
#include "auralite/ui/row.h"
#include "auralite/ui/tile.h"
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
  col->main_align(Align::Center);

  auto a = std::make_unique<Button>();
  a->text(L"a");
  a->fixed_width(40.f);
  a->fixed_height(20.f);
  a->hug_width();
  Button* pa = a.get();
  col->AddChild(std::move(a));
  col->Layout(RectF{0, 0, 100, 100});

  ExpectNear("main_align center y", pa->bounds().y, 40.f);
}

}  // namespace

int main() {
  TestRowWeight();
  TestWeightIgnoredWithoutFill();
  TestAbsoluteDualAnchor();
  TestAbsoluteBottomRight();
  TestColumnMainAlignCenter();
  TestClipDefaults();
  TestClipYaml();
  TestHitTestStillClipsToBounds();
  TestTooltipResolve();
  if (g_failures > 0) {
    std::printf("%d failure(s)\n", g_failures);
    return 1;
  }
  std::printf("all layout tests passed\n");
  return 0;
}
