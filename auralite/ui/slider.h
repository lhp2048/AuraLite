#pragma once

#include "auralite/ui/node.h"

#include <functional>

namespace auralite::ui {

// Horizontal slider; value in [0,1].
class Slider : public Node {
 public:
  using ChangeHandler = std::function<void(float)>;

  Slider();

  Slider& value(float v);
  float value() const { return value_; }
  Slider& on_changed(ChangeHandler handler);

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;

 private:
  static constexpr float kThumbR = 8.f;
  float TrackLeft() const;
  float TrackWidth() const;
  void SetValueFromX(float x);
  void Notify();

  float value_ = 0.f;
  bool dragging_ = false;
  ChangeHandler on_changed_;
};

}  // namespace auralite::ui
