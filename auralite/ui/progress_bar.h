#pragma once

#include "auralite/ui/node.h"

namespace auralite::ui {

// Determinate progress track; value in [0,1].
class ProgressBar : public Node {
 public:
  ProgressBar();

  ProgressBar& value(float v);
  float value() const { return value_; }

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

 private:
  float value_ = 0.f;
};

}  // namespace auralite::ui
