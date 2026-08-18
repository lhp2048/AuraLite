// Console tests for SpinBox / DatePicker / MenuBar / StatusBar (no HWND).
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "auralite/ui/civil_date.h"
#include "auralite/ui/date_picker.h"
#include "auralite/ui/button.h"
#include "auralite/ui/column.h"
#include "auralite/ui/factory.h"
#include "auralite/ui/menu_bar.h"
#include "auralite/ui/menu_item.h"
#include "auralite/ui/spin_box.h"
#include "auralite/ui/status_bar.h"
#include "auralite/ui/yaml_loader.h"

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

void TestCivilDate() {
  using namespace auralite::ui;
  Expect("leap 2024", IsLeapYear(2024));
  Expect("not leap 2025", !IsLeapYear(2025));
  Expect("feb 2024", DaysInMonth(2024, 2) == 29);
  Expect("feb 2025", DaysInMonth(2025, 2) == 28);
  Expect("add month jan 31", AddMonths(CivilDate{2026, 1, 31}, 1) ==
                                 CivilDate{2026, 2, 28});
  Expect("format", FormatYmd(CivilDate{2026, 8, 18}) == L"2026-08-18");
  Expect("format hms",
         FormatYmdHms(CivilDate{2026, 8, 18}, 19, 30, 5) == L"2026-08-18 19:30:05");
  CivilDate d;
  Expect("parse", ParseYmd(L"2026-08-18", &d) && d == CivilDate{2026, 8, 18});
  Expect("weekday monday-based 2026-08-17 is Mon",
         Weekday(CivilDate{2026, 8, 17}) == 1);
}

void TestSpinBox() {
  using namespace auralite::ui;
  SpinBox spin;
  spin.min_value(0).max_value(10).step(1).value(3);
  Expect("spin role", spin.acc_role() == AccRole::Spinner);
  Expect("spin value", spin.value() == 3.0);
  Expect("spin clamp", spin.value(99).value() == 10.0);
  Expect("spin set range", spin.AccSetRangeValue(4) && spin.value() == 4.0);
  Expect("spin acc value", spin.AccValue() == L"4");
}

void TestDatePicker() {
  using namespace auralite::ui;
  DatePicker dp;
  dp.year(2026).month(8).day(18);
  Expect("date role combo", dp.acc_role() == AccRole::ComboBox);
  Expect("date value", dp.AccValue() == L"2026-08-18");
  Expect("date set", dp.AccSetValue(L"2026-01-02") &&
                         dp.date() == CivilDate{2026, 1, 2});
  dp.time(true).hour(19).minute(30);
  Expect("datetime value", dp.AccValue() == L"2026-01-02 19:30");
  dp.seconds(true).second(45);
  Expect("datetime seconds", dp.AccValue() == L"2026-01-02 19:30:45");
  Expect("datetime set seconds",
         dp.AccSetValue(L"2026-03-04 08:09:10") && dp.second() == 10 &&
             dp.hour() == 8 && dp.minute() == 9);
  Expect("collapsed", !dp.AccIsExpanded());
}

void TestBars() {
  using namespace auralite::ui;
  MenuBar bar;
  bar.add_menu(L"文件", {{L"打开"}, {L"-", {}, true}, {L"退出"}});
  bar.add_menu(L"编辑", {{L"复制"}});
  Expect("menubar role", bar.acc_role() == AccRole::MenuBar);
  Expect("menubar count", bar.menu_count() == 2);
  Expect("menubar title", bar.menu_title(0) == L"文件");

  MenuItem check;
  check.text(L"状态栏").checkable(true).checked(true);
  Expect("menuitem check role", check.acc_role() == AccRole::CheckBox);
  Expect("menuitem checked", check.checked());
  MenuItem radio;
  radio.text(L"列表").radio_group(1).checked(true);
  Expect("menuitem radio role", radio.acc_role() == AccRole::RadioButton);
  MenuItem icon;
  icon.text(L"打开").icon(L"folder");
  Expect("menuitem icon", icon.icon() == L"folder");
  MenuItem sep;
  sep.separator(true);
  Expect("menuitem sep role", sep.acc_role() == AccRole::Ignore);

  StatusBar status;
  status.items({L"就绪", L"UTF-8"});
  Expect("statusbar role", status.acc_role() == AccRole::StatusBar);
  Expect("statusbar panes", status.item_count() == 2);
  Expect("statusbar value", status.AccValue() == L"就绪");
  status.set_item(0, L"已保存");
  Expect("statusbar set", status.AccValue() == L"已保存");
}

void TestYaml() {
  using namespace auralite::ui;
  ViewFactory factory;
  auto spin = LoadYamlString("SpinBox:\n  value: 3\n  min: 0\n  max: 10\n",
                             factory, {});
  Expect("yaml spin", spin && dynamic_cast<SpinBox*>(spin.get()) &&
                          dynamic_cast<SpinBox*>(spin.get())->value() == 3.0);

  auto dp = LoadYamlString(
      "DatePicker:\n  year: 2026\n  month: 8\n  day: 18\n", factory, {});
  Expect("yaml date",
         dp && dynamic_cast<DatePicker*>(dp.get()) &&
             dynamic_cast<DatePicker*>(dp.get())->date() ==
                 CivilDate{2026, 8, 18});

  auto dps = LoadYamlString(
      "DatePicker:\n  year: 2026\n  month: 8\n  day: 18\n  time: true\n"
      "  seconds: true\n  hour: 19\n  minute: 30\n  second: 45\n",
      factory, {});
  auto* dtp = dynamic_cast<DatePicker*>(dps.get());
  Expect("yaml datetime seconds",
         dtp && dtp->seconds() && dtp->second() == 45 &&
             dtp->AccValue() == L"2026-08-18 19:30:45");

  auto mb = LoadYamlString(
      "MenuBar:\n  items:\n    - text: 文件\n      items:\n        - { text: 打开 }\n",
      factory, {});
  Expect("yaml menubar", mb && dynamic_cast<MenuBar*>(mb.get()) &&
                             dynamic_cast<MenuBar*>(mb.get())->menu_count() == 1);

  auto mb_chrome = LoadYamlString(
      "MenuBar:\n  corner_radius: 12\n  border_width: 2\n  items:\n"
      "    - text: 文件\n      items:\n        - { text: 打开 }\n",
      factory, {});
  auto* chrome = dynamic_cast<MenuBar*>(mb_chrome.get());
  Expect("yaml menubar chrome", chrome && chrome->corner_radius() == 12.f &&
                                    chrome->border_width() == 2.f);

  auto mi = LoadYamlString(
      "MenuItem:\n  text: 打开\n  icon: folder\n  checkable: true\n  checked: true\n",
      factory, {});
  auto* item = dynamic_cast<MenuItem*>(mi.get());
  Expect("yaml menuitem", item && item->text() == L"打开" && item->icon() == L"folder" &&
                              item->checkable() && item->checked());

  auto mi_radio = LoadYamlString(
      "MenuItem:\n  text: 列表\n  radio_group: 1\n  checked: true\n", factory, {});
  auto* radio_item = dynamic_cast<MenuItem*>(mi_radio.get());
  Expect("yaml menuitem radio",
         radio_item && radio_item->radio_group() == 1 && radio_item->checked() &&
             radio_item->acc_role() == AccRole::RadioButton);

  auto mb_items = LoadYamlString(
      "MenuBar:\n  items:\n    - text: 查看\n      items:\n"
      "        - { text: 状态栏, checkable: true, checked: true }\n"
      "        - { text: \"-\" }\n"
      "        - { text: 列表, radio_group: 1, checked: true, icon: list }\n",
      factory, {});
  Expect("yaml menubar check radio",
         mb_items && dynamic_cast<MenuBar*>(mb_items.get()) &&
             dynamic_cast<MenuBar*>(mb_items.get())->menu_count() == 1);

  auto mix = LoadYamlString(
      "Column:\n  children:\n    - MenuItem: { text: 打开 }\n"
      "    - Button: { text: 自定义 }\n",
      factory, {});
  Expect("yaml menu mix",
         mix && mix->children().size() == 2 &&
             dynamic_cast<MenuItem*>(mix->children()[0].get()) &&
             dynamic_cast<Button*>(mix->children()[1].get()));

  auto sb = LoadYamlString("StatusBar:\n  items: [\"就绪\", \"UTF-8\"]\n",
                           factory, {});
  Expect("yaml status",
         sb && dynamic_cast<StatusBar*>(sb.get()) &&
             dynamic_cast<StatusBar*>(sb.get())->item_count() == 2);
}

}  // namespace

int main() {
  setvbuf(stdout, nullptr, _IONBF, 0);
  TestCivilDate();
  TestSpinBox();
  TestDatePicker();
  TestBars();
  TestYaml();
  if (g_failures) {
    std::printf("%d failed\n", g_failures);
    return EXIT_FAILURE;
  }
  std::printf("all ok\n");
  return EXIT_SUCCESS;
}
