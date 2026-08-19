#pragma once

#include "mx/ui/civil_date.h"
#include "mx/ui/node.h"

#include <functional>
#include <optional>
#include <string>

namespace mx::ui {

class Window;

// Date field with a month-grid popup (Window::SetPopup). Optional time.
class DatePicker : public Node {
 public:
  using ChangeHandler =
      std::function<void(CivilDate date, int hour, int minute, int second)>;

  DatePicker();

  void BindWindow(Window* window);

  DatePicker& date(CivilDate d);
  CivilDate date() const { return date_; }
  DatePicker& year(int y);
  DatePicker& month(int m);
  DatePicker& day(int d);
  DatePicker& time(bool enable);
  bool time() const { return time_; }
  DatePicker& seconds(bool enable);
  bool seconds() const { return seconds_; }
  DatePicker& hour(int h);
  int hour() const { return hour_; }
  DatePicker& minute(int m);
  int minute() const { return minute_; }
  DatePicker& second(int s);
  int second() const { return second_; }
  DatePicker& font_size(float size);
  DatePicker& on_changed(ChangeHandler handler);

  bool is_open() const { return open_; }
  void ClosePopup();

  AccRole acc_role() const override;
  std::wstring AccDefaultName() const override;
  std::wstring AccValue() const override;
  bool AccSetValue(const std::wstring& value) override;
  bool AccIsExpanded() const override;
  bool AccExpand() override;
  bool AccCollapse() override;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(mx::Canvas& canvas) override;
  void OnMouseDown(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  friend class CalendarPopup;
  void OpenPopup();
  void Commit(CivilDate d, int hour, int minute, int second, bool close);
  void Notify();
  std::wstring DisplayText() const;

  Window* window_ = nullptr;
  CivilDate date_{2026, 1, 1};
  bool time_ = false;
  bool seconds_ = false;
  int hour_ = 0;
  int minute_ = 0;
  int second_ = 0;
  bool open_ = false;
  std::optional<float> font_size_;
  ChangeHandler on_changed_;
};

}  // namespace mx::ui
