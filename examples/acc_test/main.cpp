// Console tests for accessibility read APIs (no UIA / HWND).
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "mx/ui/acc.h"
#include "mx/ui/button.h"
#include "mx/ui/checkbox.h"
#include "mx/ui/column.h"
#include "mx/ui/combo.h"
#include "mx/ui/factory.h"
#include "mx/ui/label.h"
#include "mx/ui/list_view.h"
#include "mx/ui/progress_bar.h"
#include "mx/ui/radio.h"
#include "mx/ui/slider.h"
#include "mx/ui/switch_control.h"
#include "mx/ui/tab.h"
#include "mx/ui/text_area.h"
#include "mx/ui/text_field.h"
#include "mx/ui/tree_view.h"
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

void TestLayoutIgnored() {
  using namespace mx::ui;
  Column col;
  Expect("column ignored", col.acc_role() == AccRole::Ignore);
  Expect("column not included", !col.AccIncluded());

  col.acc_name(L"登录");
  Expect("named column is group", col.acc_role() == AccRole::Group);
  Expect("named column included", col.AccIncluded());
  Expect("named column name", col.AccName() == L"登录");
}

void TestButton() {
  using namespace mx::ui;
  Button btn;
  btn.text(L"确定");
  Expect("button role", btn.acc_role() == AccRole::Button);
  Expect("button name from text", btn.AccName() == L"确定");
  Expect("button included", btn.AccIncluded());

  int clicks = 0;
  btn.on_click([&] { ++clicks; });
  Expect("button invoke", btn.AccInvoke());
  Expect("button invoke fired", clicks == 1);

  btn.set_enabled(false);
  Expect("disabled state", btn.acc_state().disabled);
  Expect("disabled invoke false", !btn.AccInvoke());
  Expect("no extra click", clicks == 1);

  Button named;
  named.text(L"X").acc_name(L"关闭");
  Expect("acc_name overrides text", named.AccName() == L"关闭");

  Button icon;
  icon.tooltip(L"设置");
  Expect("tooltip fallback name", icon.AccName() == L"设置");
}

void TestLabelTextField() {
  using namespace mx::ui;
  Label lab;
  lab.text(L"用户名");
  Expect("label role", lab.acc_role() == AccRole::Text);
  Expect("label name", lab.AccName() == L"用户名");

  TextField field;
  field.placeholder(L"请输入").text(L"abc");
  Expect("edit role", field.acc_role() == AccRole::Edit);
  Expect("edit name from placeholder", field.AccName() == L"请输入");
  Expect("edit value", field.AccValue() == L"abc");
  Expect("edit set value", field.AccSetValue(L"xyz"));
  Expect("edit value updated", field.text() == L"xyz");

  field.password(true);
  Expect("password state", field.acc_state().password);
  Expect("password value hidden", field.AccValue().empty());
}

void TestToggleControls() {
  using namespace mx::ui;
  Checkbox box;
  box.text(L"记住我");
  Expect("checkbox role", box.acc_role() == AccRole::CheckBox);
  Expect("checkbox name", box.AccName() == L"记住我");
  Expect("checkbox off", !box.acc_state().checked);
  Expect("checkbox toggle", box.AccToggle());
  Expect("checkbox on", box.checked() && box.acc_state().checked);

  Radio radio;
  radio.text(L"选项A");
  Expect("radio role", radio.acc_role() == AccRole::RadioButton);
  Expect("radio invoke selects", radio.AccInvoke());
  Expect("radio checked", radio.checked());

  Switch sw;
  sw.text(L"通知");
  Expect("switch as checkbox", sw.acc_role() == AccRole::CheckBox);
  Expect("switch toggle", sw.AccToggle());
  Expect("switch on", sw.is_on());
}

void TestCollectAndHidden() {
  using namespace mx::ui;
  auto root = std::make_unique<Column>();
  auto btn = std::make_unique<Button>();
  btn->text(L"A");
  auto hidden = std::make_unique<Button>();
  hidden->text(L"B");
  hidden->set_visible(false);
  auto lab = std::make_unique<Label>();
  lab->text(L"C");

  root->AddChild(std::move(btn));
  root->AddChild(std::move(hidden));
  root->AddChild(std::move(lab));

  std::vector<const Node*> acc;
  CollectAccNodes(root.get(), &acc);
  Expect("collect skips column and hidden", acc.size() == 2);
  Expect("collect button first", acc[0]->AccName() == L"A");
  Expect("collect label second", acc[1]->AccName() == L"C");
  Expect("acc id assigned", acc[0]->acc_id() != 0);
  Expect("find by id", FindAccNode(root.get(), acc[0]->acc_id()) == acc[0]);
}

void TestFindSkipsHidden() {
  using namespace mx::ui;
  auto root = std::make_unique<Column>();
  auto btn = std::make_unique<Button>();
  btn->text(L"X");
  Button* p = btn.get();
  root->AddChild(std::move(btn));

  std::vector<Node*> acc;
  CollectAccNodes(root.get(), &acc);
  Expect("collect visible button", acc.size() == 1);
  const int id = p->acc_id();
  Expect("id assigned before hide", id != 0);
  p->set_visible(false);
  Expect("find skips hidden", FindAccNode(root.get(), id) == nullptr);
}

void TestTabOffstageHidden() {
  using namespace mx::ui;
  Tab tab;
  auto page0 = std::make_unique<Column>();
  auto a = std::make_unique<Button>();
  a->text(L"A");
  page0->AddChild(std::move(a));
  auto page1 = std::make_unique<Column>();
  auto b = std::make_unique<Button>();
  b->text(L"B");
  page1->AddChild(std::move(b));
  tab.AddChild(std::move(page0));
  tab.AddChild(std::move(page1));
  tab.set_selected(0);

  std::vector<const Node*> acc;
  CollectAccNodes(&tab, &acc);
  Expect("tab role", tab.acc_role() == AccRole::Tab);
  Expect("tab included with page0", acc.size() == 2);
  Expect("tab first", acc[0] == &tab);
  Expect("tab page0 only", acc[1]->AccName() == L"A");

  tab.set_selected(1);
  acc.clear();
  CollectAccNodes(&tab, &acc);
  Expect("tab included with page1", acc.size() == 2);
  Expect("tab page1 only", acc[1]->AccName() == L"B");
  tab.add_header(L"甲");
  tab.add_header(L"乙");
  Expect("tab value header", tab.AccValue() == L"乙");
  Expect("tab set value", tab.AccSetValue(L"甲") && tab.selected() == 0);
}

void TestYamlAccName() {
  using namespace mx::ui;
  ViewFactory factory;
  auto n = LoadYamlString(
      "Button:\n  text: OK\n  acc_name: 确认\n", factory, {});
  Expect("yaml built", n != nullptr);
  Expect("yaml acc_name", n && n->AccName() == L"确认");
  Expect("yaml role", n && n->acc_role() == AccRole::Button);

  auto menu = LoadYamlString(
      "Button:\n  text: Refresh\n  acc_role: menuitem\n", factory, {});
  Expect("yaml menuitem role",
         menu && menu->acc_role() == AccRole::MenuItem);
  Expect("yaml menuitem name", menu && menu->AccName() == L"Refresh");
  int clicks = 0;
  if (auto* btn = dynamic_cast<Button*>(menu.get())) {
    btn->on_click([&] { ++clicks; });
  }
  Expect("yaml menuitem invoke", menu && menu->AccInvoke() && clicks == 1);
}

void TestComboTextArea() {
  using namespace mx::ui;
  Combo combo;
  combo.items({L"红", L"绿"});
  combo.selected(1);
  Expect("combo role", combo.acc_role() == AccRole::ComboBox);
  Expect("combo name selected", combo.AccName() == L"绿");
  Expect("combo value selected", combo.AccValue() == L"绿");
  Expect("combo set value", combo.AccSetValue(L"红") && combo.selected() == 0);
  Expect("combo collapsed", !combo.AccIsExpanded());

  TextArea area;
  area.placeholder(L"备注").text(L"hello");
  Expect("textarea edit", area.acc_role() == AccRole::Edit);
  Expect("textarea name", area.AccName() == L"备注");
  Expect("textarea value", area.AccValue() == L"hello");
}

void TestRangeListRoles() {
  using namespace mx::ui;
  Slider slider;
  slider.value(0.25f).step(0.1f);
  Expect("slider role", slider.acc_role() == AccRole::Slider);
  Expect("slider range", std::abs(slider.AccRangeValue() - 0.25) < 1e-6);
  Expect("slider set range", slider.AccSetRangeValue(0.8) &&
                                 std::abs(slider.value() - 0.8f) < 1e-6);
  Expect("slider writable", !slider.AccRangeReadOnly());

  ProgressBar bar;
  bar.value(0.4f);
  Expect("progress role", bar.acc_role() == AccRole::ProgressBar);
  Expect("progress readonly", bar.AccRangeReadOnly());
  Expect("progress set denied", !bar.AccSetRangeValue(0.9));

  ListView list;
  Expect("list role", list.acc_role() == AccRole::List);
  TreeView tree;
  Expect("tree role", tree.acc_role() == AccRole::Tree);
}

}  // namespace

int main() {
  setvbuf(stdout, nullptr, _IONBF, 0);
  TestLayoutIgnored();
  TestButton();
  TestLabelTextField();
  TestToggleControls();
  TestCollectAndHidden();
  TestFindSkipsHidden();
  TestTabOffstageHidden();
  TestYamlAccName();
  TestComboTextArea();
  TestRangeListRoles();
  if (g_failures) {
    std::printf("%d failed\n", g_failures);
    return EXIT_FAILURE;
  }
  std::printf("all ok\n");
  return EXIT_SUCCESS;
}
