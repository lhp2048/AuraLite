#pragma once

#include "auralite/ui/anim.h"
#include "auralite/ui/node.h"

namespace auralite::ui {

class Window;

// Progress track; value in [0,1]. Supports indeterminate animation.
class ProgressBar : public Node {
 public:
  ProgressBar();
  ~ProgressBar() override;

  void BindWindow(Window* window);

  ProgressBar& value(float v);
  float value() const { return value_; }
  float visual_value() const { return visual_value_; }
  ProgressBar& indeterminate(bool enable);
  bool indeterminate() const { return indeterminate_; }

  AccRole acc_role() const override;
  double AccRangeValue() const override;
  bool AccRangeReadOnly() const override;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

 private:
  void SyncAnimation();
  void SyncVisual(bool instant);
  Window* AnimWindow() const;
  void OnAnimateChanged() override;
  void OnHostWindowChanged() override;

  Window* window_ = nullptr;
  float value_ = 0.f;
  float visual_value_ = 0.f;
  Tween value_tween_;
  bool indeterminate_ = false;
  bool anim_registered_ = false;
};

}  // namespace auralite::ui
