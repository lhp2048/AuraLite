#include "mx/ui/horizontal_scrollbar.h"

#include "mx/canvas.h"
#include "mx/ui/theme.h"

#include <algorithm>

namespace mx::ui {

bool HorizontalScrollbar::needed() const {
  return content_w_ > viewport_w_ && viewport_w_ > 0.f;
}

float HorizontalScrollbar::max_scroll() const {
  return std::max(0.f, content_w_ - viewport_w_);
}

void HorizontalScrollbar::ClampScroll() {
  scroll_x_ = std::clamp(scroll_x_, 0.f, max_scroll());
}

void HorizontalScrollbar::set_scroll_offset(float x) {
  scroll_x_ = x;
  ClampScroll();
}

RectF HorizontalScrollbar::thumb_bounds() const {
  const float track_w = track_.w;
  const float content_w = std::max(1.f, content_w_);
  float thumb_w =
      (viewport_w_ > 0.f) ? (track_w * viewport_w_ / content_w) : 0.f;
  thumb_w = std::max(kMinThumbWidth, std::min(track_w, thumb_w));
  const float max_s = max_scroll();
  const float travel = std::max(0.f, track_w - thumb_w);
  const float thumb_x =
      (max_s > 0.f) ? (travel * scroll_x_ / max_s) : 0.f;
  return RectF{track_.x + thumb_x, track_.y, thumb_w, track_.h};
}

float HorizontalScrollbar::ScrollOffsetFromThumbX(float thumb_x) const {
  const float track_w = track_.w;
  const float thumb_w = thumb_bounds().w;
  const float travel = std::max(1.f, track_w - thumb_w);
  const float x = std::clamp(thumb_x - track_.x, 0.f, travel);
  return max_scroll() * x / travel;
}

void HorizontalScrollbar::Paint(mx::Canvas& canvas) const {
  if (!needed()) {
    return;
  }
  const auto& t = Theme::Active();
  canvas.FillRect(track_, t.scroll_track);
  canvas.FillRect(thumb_bounds(), t.scroll_thumb);
}

bool HorizontalScrollbar::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left || !needed()) {
    return false;
  }
  const RectF thumb = thumb_bounds();
  if (e.x >= thumb.x && e.x < thumb.x + thumb.w && e.y >= thumb.y &&
      e.y < thumb.y + thumb.h) {
    dragging_ = true;
    drag_anchor_x_ = e.x;
    drag_scroll_anchor_ = scroll_x_;
    return true;
  }
  if (e.x >= track_.x && e.x < track_.x + track_.w && e.y >= track_.y &&
      e.y < track_.y + track_.h) {
    set_scroll_offset(ScrollOffsetFromThumbX(e.x - thumb.w * 0.5f));
    dragging_ = true;
    drag_anchor_x_ = e.x;
    drag_scroll_anchor_ = scroll_x_;
    return true;
  }
  return false;
}

bool HorizontalScrollbar::OnMouseMove(const MouseEvent& e) {
  if (!dragging_) {
    return false;
  }
  const float delta = e.x - drag_anchor_x_;
  const float track_w = track_.w;
  const float thumb_w = thumb_bounds().w;
  const float travel = std::max(1.f, track_w - thumb_w);
  set_scroll_offset(drag_scroll_anchor_ + max_scroll() * delta / travel);
  return true;
}

bool HorizontalScrollbar::OnMouseUp(const MouseEvent&) {
  if (!dragging_) {
    return false;
  }
  dragging_ = false;
  return true;
}

void HorizontalScrollbar::CancelDrag() { dragging_ = false; }

}  // namespace mx::ui
