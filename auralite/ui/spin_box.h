#pragma once

#include "auralite/ui/node.h"

#include <functional>
#include <optional>
#include <string>

namespace auralite::ui {

// Numeric stepper. Keyboard Up/Down, mouse wheel, click the chevrons.
class SpinBox : public Node {
 public:
  using ChangeHandler = std::function<void(double value)>;

  SpinBox();

  SpinBox& value(double v);
  double value() const { return value_; }
  SpinBox& min_value(double v);
  double min_value() const { return min_; }
  SpinBox& max_value(double v);
  double max_value() const { return max_; }
  SpinBox& step(double s);
  double step() const { return step_; }
  SpinBox& decimals(int n);
  int decimals() const { return decimals_; }
  SpinBox& wrap(bool enable);
  bool wrap() const { return wrap_; }
  SpinBox& font_size(float size);
  SpinBox& on_changed(ChangeHandler handler);

  AccRole acc_role() const override;
  std::wstring AccDefaultName() const override;
  std::wstring AccValue() const override;
  bool AccSetValue(const std::wstring& value) override;
  double AccRangeValue() const override;
  double AccRangeMinimum() const override;
  double AccRangeMaximum() const override;
  double AccRangeSmallChange() const override;
  double AccRangeLargeChange() const override;
  bool AccRangeReadOnly() const override;
  bool AccSetRangeValue(double value) override;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;
  bool WantsMouseWheel() const override { return true; }
  void OnMouseWheel(const MouseEvent& e) override;

 private:
  static constexpr float kChevronW = 22.f;
  RectF ChevronRect() const;
  int HitChevron(float x, float y) const;  // +1 up, -1 down, 0 miss
  void Nudge(int dir, bool large);
  void SetClamped(double v, bool notify);
  std::wstring FormatValue() const;

  double value_ = 0.0;
  double min_ = 0.0;
  double max_ = 100.0;
  double step_ = 1.0;
  int decimals_ = 0;
  bool wrap_ = false;
  int hover_dir_ = 0;
  std::optional<float> font_size_;
  ChangeHandler on_changed_;
};

}  // namespace auralite::ui
