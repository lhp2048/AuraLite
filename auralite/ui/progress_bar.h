#pragma once

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
  ProgressBar& indeterminate(bool enable);
  bool indeterminate() const { return indeterminate_; }

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

 private:
  void SyncAnimation();

  Window* window_ = nullptr;
  float value_ = 0.f;
  bool indeterminate_ = false;
  bool anim_registered_ = false;
};

}  // namespace auralite::ui
