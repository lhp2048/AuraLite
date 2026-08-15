// Console layout smoke tests for Column/Row weight and Absolute anchors.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

#include "auralite/ui/absolute.h"
#include "auralite/ui/button.h"
#include "auralite/ui/column.h"
#include "auralite/ui/row.h"

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
  if (g_failures > 0) {
    std::printf("%d failure(s)\n", g_failures);
    return 1;
  }
  std::printf("all layout tests passed\n");
  return 0;
}
