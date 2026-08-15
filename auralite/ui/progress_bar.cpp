#include "auralite/ui/progress_bar.h"

#include <algorithm>

namespace auralite::ui {

ProgressBar::ProgressBar() {
  fill_width();
  fixed_height(12.f);
}

ProgressBar& ProgressBar::value(float v) {
  value_ = std::clamp(v, 0.f, 1.f);
  return *this;
}

SizeF ProgressBar::Measure(float max_w, float max_h) {
  const float hug_h = preferred_height() > 0.f ? preferred_height() : 12.f;
  return ResolveSize(max_w, max_h, preferred_width() > 0.f ? preferred_width()
                                                          : max_w,
                     hug_h);
}

void ProgressBar::Paint(auralite::Canvas& canvas) {
  const float r = bounds_.h * 0.5f;
  canvas.FillRoundedRect(bounds_, r, r, ColorF::FromRgb(220, 226, 235));
  if (value_ <= 0.f) {
    return;
  }
  RectF fill = bounds_;
  fill.w = std::max(0.f, bounds_.w * value_);
  canvas.FillRoundedRect(fill, r, r, ColorF::FromRgb(40, 110, 200));
}

}  // namespace auralite::ui
