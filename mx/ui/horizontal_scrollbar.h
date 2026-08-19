#pragma once

#include "mx/ui/types.h"

namespace mx {
class Canvas;
}

namespace mx::ui {

// Shared horizontal scrollbar geometry + drag (lists with wide columns).
class HorizontalScrollbar {
 public:
  static constexpr float kHeight = 10.f;
  static constexpr float kMinThumbWidth = 20.f;

  void set_content_width(float w) { content_w_ = (w > 0.f) ? w : 0.f; }
  void set_viewport_width(float w) { viewport_w_ = (w > 0.f) ? w : 0.f; }
  void set_scroll_offset(float x);
  float scroll_offset() const { return scroll_x_; }

  // Track rectangle in window/canvas coordinates (typically bottom strip).
  void set_track_bounds(const RectF& track) { track_ = track; }
  const RectF& track_bounds() const { return track_; }

  bool needed() const;
  float max_scroll() const;
  void ClampScroll();

  RectF thumb_bounds() const;
  float ScrollOffsetFromThumbX(float thumb_x) const;

  void Paint(mx::Canvas& canvas) const;

  // Returns true if the event was handled (thumb/track drag).
  bool OnMouseDown(const MouseEvent& e);
  bool OnMouseMove(const MouseEvent& e);
  bool OnMouseUp(const MouseEvent& e);
  void CancelDrag();

  bool dragging() const { return dragging_; }

 private:
  float content_w_ = 0.f;
  float viewport_w_ = 0.f;
  float scroll_x_ = 0.f;
  RectF track_{};
  bool dragging_ = false;
  float drag_anchor_x_ = 0.f;
  float drag_scroll_anchor_ = 0.f;
};

}  // namespace mx::ui
