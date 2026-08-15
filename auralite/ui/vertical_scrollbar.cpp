#include "auralite/ui/vertical_scrollbar.h"

#include "auralite/canvas.h"
#include "auralite/ui/theme.h"

#include <algorithm>

namespace auralite::ui {

bool VerticalScrollbar::needed() const {
  return content_h_ > viewport_h_ && viewport_h_ > 0.f;
}

float VerticalScrollbar::max_scroll() const {
  return std::max(0.f, content_h_ - viewport_h_);
}

void VerticalScrollbar::ClampScroll() {
  scroll_y_ = std::clamp(scroll_y_, 0.f, max_scroll());
}

void VerticalScrollbar::set_scroll_offset(float y) {
  scroll_y_ = y;
  ClampScroll();
}

RectF VerticalScrollbar::thumb_bounds() const {
  const float track_h = track_.h;
  const float content_h = std::max(1.f, content_h_);
  float thumb_h = (viewport_h_ > 0.f) ? (track_h * viewport_h_ / content_h) : 0.f;
  thumb_h = std::max(kMinThumbHeight, std::min(track_h, thumb_h));
  const float max_s = max_scroll();
  const float travel = std::max(0.f, track_h - thumb_h);
  const float thumb_y =
      (max_s > 0.f) ? (travel * scroll_y_ / max_s) : 0.f;
  return RectF{track_.x, track_.y + thumb_y, track_.w, thumb_h};
}

float VerticalScrollbar::ScrollOffsetFromThumbY(float thumb_y) const {
  const float track_h = track_.h;
  const float thumb_h = thumb_bounds().h;
  const float travel = std::max(1.f, track_h - thumb_h);
  const float y = std::clamp(thumb_y - track_.y, 0.f, travel);
  return max_scroll() * y / travel;
}

void VerticalScrollbar::Paint(auralite::Canvas& canvas) const {
  if (!needed()) {
    return;
  }
  const auto& t = Theme::Active();
  canvas.FillRect(track_, t.scroll_track);
  canvas.FillRect(thumb_bounds(), t.scroll_thumb);
}

bool VerticalScrollbar::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left || !needed()) {
    return false;
  }
  const RectF thumb = thumb_bounds();
  if (e.x >= thumb.x && e.x < thumb.x + thumb.w && e.y >= thumb.y &&
      e.y < thumb.y + thumb.h) {
    dragging_ = true;
    drag_anchor_y_ = e.y;
    drag_scroll_anchor_ = scroll_y_;
    return true;
  }
  if (e.x >= track_.x && e.x < track_.x + track_.w && e.y >= track_.y &&
      e.y < track_.y + track_.h) {
    set_scroll_offset(ScrollOffsetFromThumbY(e.y - thumb.h * 0.5f));
    dragging_ = true;
    drag_anchor_y_ = e.y;
    drag_scroll_anchor_ = scroll_y_;
    return true;
  }
  return false;
}

bool VerticalScrollbar::OnMouseMove(const MouseEvent& e) {
  if (!dragging_) {
    return false;
  }
  const float delta = e.y - drag_anchor_y_;
  const float track_h = track_.h;
  const float thumb_h = thumb_bounds().h;
  const float travel = std::max(1.f, track_h - thumb_h);
  set_scroll_offset(drag_scroll_anchor_ + max_scroll() * delta / travel);
  return true;
}

bool VerticalScrollbar::OnMouseUp(const MouseEvent&) {
  if (!dragging_) {
    return false;
  }
  dragging_ = false;
  return true;
}

void VerticalScrollbar::CancelDrag() { dragging_ = false; }

}  // namespace auralite::ui
