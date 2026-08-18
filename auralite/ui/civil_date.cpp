#include "auralite/ui/civil_date.h"

#include <algorithm>
#include <chrono>
#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace auralite::ui {
namespace {

std::chrono::year_month_day ToYmd(CivilDate d) {
  using namespace std::chrono;
  return year{d.year} / month{static_cast<unsigned>(d.month)} /
         day{static_cast<unsigned>(d.day)};
}

CivilDate FromYmd(std::chrono::year_month_day ymd) {
  return CivilDate{static_cast<int>(ymd.year()),
                   static_cast<int>(static_cast<unsigned>(ymd.month())),
                   static_cast<int>(static_cast<unsigned>(ymd.day()))};
}

}  // namespace

bool IsLeapYear(int year) {
  return std::chrono::year{year}.is_leap();
}

int DaysInMonth(int y, int m) {
  using namespace std::chrono;
  m = std::clamp(m, 1, 12);
  const year_month_day_last ymdl{std::chrono::year{y} /
                                 std::chrono::month{static_cast<unsigned>(m)} /
                                 std::chrono::last};
  return static_cast<int>(static_cast<unsigned>(ymdl.day()));
}

CivilDate NormalizeDate(CivilDate d) {
  d.month = std::clamp(d.month, 1, 12);
  const int dim = DaysInMonth(d.year, d.month);
  d.day = std::clamp(d.day, 1, dim);
  return d;
}

int Weekday(CivilDate d) {
  using namespace std::chrono;
  d = NormalizeDate(d);
  const weekday wd{sys_days{ToYmd(d)}};
  return static_cast<int>(wd.c_encoding());
}

CivilDate AddDays(CivilDate d, int n) {
  using namespace std::chrono;
  d = NormalizeDate(d);
  return FromYmd(year_month_day{sys_days{ToYmd(d)} + days{n}});
}

CivilDate AddMonths(CivilDate d, int n) {
  using namespace std::chrono;
  d = NormalizeDate(d);
  year_month ym = year{d.year} / month{static_cast<unsigned>(d.month)};
  ym += months{n};
  const unsigned dim =
      static_cast<unsigned>((ym / last).day());
  const unsigned day =
      std::min(static_cast<unsigned>(d.day), dim);
  return FromYmd(ym / std::chrono::day{day});
}

CivilDate TodayLocal() {
  SYSTEMTIME st = {};
  GetLocalTime(&st);
  return NormalizeDate(
      CivilDate{st.wYear, st.wMonth, st.wDay});
}

std::wstring FormatYmd(CivilDate d) {
  d = NormalizeDate(d);
  wchar_t buf[32] = {};
  swprintf_s(buf, L"%04d-%02d-%02d", d.year, d.month, d.day);
  return buf;
}

std::wstring FormatYmdHm(CivilDate d, int hour, int minute) {
  hour = std::clamp(hour, 0, 23);
  minute = std::clamp(minute, 0, 59);
  wchar_t buf[40] = {};
  swprintf_s(buf, L"%s %02d:%02d", FormatYmd(d).c_str(), hour, minute);
  return buf;
}

std::wstring FormatYmdHms(CivilDate d, int hour, int minute, int second) {
  hour = std::clamp(hour, 0, 23);
  minute = std::clamp(minute, 0, 59);
  second = std::clamp(second, 0, 59);
  wchar_t buf[48] = {};
  swprintf_s(buf, L"%s %02d:%02d:%02d", FormatYmd(d).c_str(), hour, minute,
              second);
  return buf;
}

bool ParseYmd(const std::wstring& s, CivilDate* out) {
  if (!out || s.size() < 8) {
    return false;
  }
  int y = 0;
  int m = 0;
  int day = 0;
  if (swscanf_s(s.c_str(), L"%d-%d-%d", &y, &m, &day) != 3) {
    return false;
  }
  if (m < 1 || m > 12) {
    return false;
  }
  *out = NormalizeDate(CivilDate{y, m, day});
  return true;
}

}  // namespace auralite::ui
