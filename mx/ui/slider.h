#pragma once

#include "mx/ui/anim.h"
#include "mx/ui/node.h"

#include <functional>

namespace mx::ui {

enum class SliderOrientation { Horizontal, Vertical };

// Slider; value in [0,1]. Supports horizontal/vertical + keyboard + ticks.
class Slider : public Node {
 public:
  using ChangeHandler = std::function<void(float)>;

  Slider();

  Slider& value(float v);
  float value() const { return value_; }
  float visual_value() const { return visual_value_; }
  Slider& orientation(SliderOrientation o);
  SliderOrientation orientation() const { return orientation_; }
  // Keyboard / Page step; 0 = default 0.05.
  Slider& step(float s);
  float step() const { return step_; }
  // n<=1: no ticks; n>=2: n marks including endpoints.
  Slider& tick_count(int n);
  int tick_count() const { return tick_count_; }
  Slider& on_changed(ChangeHandler handler);

  AccRole acc_role() const override;
  double AccRangeValue() const override;
  double AccRangeSmallChange() const override;
  double AccRangeLargeChange() const override;
  bool AccRangeReadOnly() const override;
  bool AccSetRangeValue(double value) override;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(mx::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  static constexpr float kThumbR = 8.f;
  bool IsVertical() const {
    return orientation_ == SliderOrientation::Vertical;
  }
  float TrackOrigin() const;
  float TrackLength() const;
  void SetValueFromPointer(float x, float y);
  void AdjustValue(float delta);
  void Notify();
  void ApplyDefaultSize();
  void SyncVisual(bool instant);
  void OnAnimateChanged() override;
  void OnHostWindowChanged() override;

  float value_ = 0.f;
  float visual_value_ = 0.f;
  Tween value_tween_;
  float step_ = 0.05f;
  int tick_count_ = 0;
  bool dragging_ = false;
  SliderOrientation orientation_ = SliderOrientation::Horizontal;
  ChangeHandler on_changed_;
};

}  // namespace mx::ui
