#pragma once

#include <string>

namespace mx::ui {

// Gregorian date. month is 1–12.
struct CivilDate {
  int year = 1970;
  int month = 1;
  int day = 1;
};

inline bool operator==(CivilDate a, CivilDate b) {
  return a.year == b.year && a.month == b.month && a.day == b.day;
}

inline bool operator!=(CivilDate a, CivilDate b) { return !(a == b); }

bool IsLeapYear(int year);
int DaysInMonth(int year, int month);
// 0 = Sunday … 6 = Saturday. Invalid dates are normalized first.
int Weekday(CivilDate d);
CivilDate NormalizeDate(CivilDate d);
CivilDate AddDays(CivilDate d, int n);
CivilDate AddMonths(CivilDate d, int n);
CivilDate TodayLocal();

std::wstring FormatYmd(CivilDate d);
std::wstring FormatYmdHm(CivilDate d, int hour, int minute);
std::wstring FormatYmdHms(CivilDate d, int hour, int minute, int second);
bool ParseYmd(const std::wstring& s, CivilDate* out);

}  // namespace mx::ui
