#include "mx/ui/progress_bar.h"

#include "mx/ui/theme.h"
#include "mx/ui/window.h"

#include <algorithm>

namespace mx::ui {

ProgressBar::ProgressBar() {
  fill_width();
  fixed_height(12.f);
}

ProgressBar::~ProgressBar() {
  if (anim_registered_ && window_) {
    window_->UnregisterAnimation();
    anim_registered_ = false;
  }
}

void ProgressBar::BindWindow(Window* window) {
  if (anim_registered_ && window_) {
    window_->UnregisterAnimation();
    anim_registered_ = false;
  }
  window_ = window;
  SyncAnimation();
}

ProgressBar& ProgressBar::value(float v) {
  v = std::clamp(v, 0.f, 1.f);
  if (v == value_) {
    return *this;
  }
  value_ = v;
  if (!indeterminate_) {
    SyncVisual(false);
  } else {
    visual_value_ = value_;
  }
  NotifyAccRangeChanged();
  return *this;
}

ProgressBar& ProgressBar::indeterminate(bool enable) {
  indeterminate_ = enable;
  SyncAnimation();
  return *this;
}

void ProgressBar::SyncAnimation() {
  const bool want = indeterminate_ && window_ != nullptr;
  if (want == anim_registered_) {
    return;
  }
  if (want) {
    window_->RegisterAnimation();
    anim_registered_ = true;
  } else if (window_) {
    window_->UnregisterAnimation();
    anim_registered_ = false;
  }
}

SizeF ProgressBar::Measure(float max_w, float max_h) {
  const float hug_h = preferred_height() > 0.f ? preferred_height() : 12.f;
  return ResolveSize(max_w, max_h, preferred_width() > 0.f ? preferred_width()
                                                          : max_w,
                     hug_h);
}

void ProgressBar::Paint(mx::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  const float r = bounds_.h * 0.5f;
  canvas.FillRoundedRect(bounds_, r, r, th.scroll_track);

  if (indeterminate_) {
    const float pulse_w = std::max(24.f, bounds_.w * 0.28f);
    const float travel = std::max(0.f, bounds_.w - pulse_w);
    const float t =
        static_cast<float>(GetTickCount64() % 1400ULL) / 1400.f;
    // Ease back and forth.
    const float phase = (t < 0.5f) ? (t * 2.f) : (2.f - t * 2.f);
    RectF fill = bounds_;
    fill.x = bounds_.x + travel * phase;
    fill.w = pulse_w;
    canvas.FillRoundedRect(fill, r, r, th.accent);
    return;
  }

  if (value_ <= 0.f && visual_value_ <= 0.f) {
    return;
  }
  RectF fill = bounds_;
  fill.w = std::max(0.f, bounds_.w * visual_value_);
  canvas.FillRoundedRect(fill, r, r, th.accent);
}

Window* ProgressBar::AnimWindow() const {
  return host_window() ? host_window() : window_;
}

void ProgressBar::SyncVisual(bool instant) {
  const float to = value_;
  Window* w = AnimWindow();
  if (instant || !animate() || !w || !w->hwnd()) {
    value_tween_.Cancel();
    visual_value_ = to;
    return;
  }
  const float from = visual_value_;
  if (from == to) {
    return;
  }
  value_tween_.Start(
      w, kUiAnimSec, Easing::EaseOutCubic,
      [this, from, to](float t) { visual_value_ = from + (to - from) * t; },
      [this, to] { visual_value_ = to; });
}

void ProgressBar::OnAnimateChanged() { SyncVisual(true); }

void ProgressBar::OnHostWindowChanged() {
  if (!CanTween() && !(window_ && window_->hwnd() && animate())) {
    SyncVisual(true);
  }
}

AccRole ProgressBar::acc_role() const {
  if (acc_role_override_) {
    return *acc_role_override_;
  }
  return AccRole::ProgressBar;
}

double ProgressBar::AccRangeValue() const {
  return value_;
}

bool ProgressBar::AccRangeReadOnly() const {
  return true;
}

}  // namespace mx::ui
