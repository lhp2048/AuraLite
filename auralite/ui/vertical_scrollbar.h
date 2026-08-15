#pragma once

#include "auralite/ui/types.h"

namespace auralite {
class Canvas;
}

namespace auralite::ui {

// Shared vertical scrollbar geometry + drag (used by lists / tree / ScrollView).
class VerticalScrollbar {
 public:
  static constexpr float kWidth = 10.f;
  static constexpr float kMinThumbHeight = 20.f;

  void set_content_height(float h) { content_h_ = (h > 0.f) ? h : 0.f; }
  void set_viewport_height(float h) { viewport_h_ = (h > 0.f) ? h : 0.f; }
  void set_scroll_offset(float y);
  float scroll_offset() const { return scroll_y_; }

  // Track rectangle in window/canvas coordinates (typically right strip).
  void set_track_bounds(const RectF& track) { track_ = track; }
  const RectF& track_bounds() const { return track_; }

  bool needed() const;
  float max_scroll() const;
  void ClampScroll();

  RectF thumb_bounds() const;
  float ScrollOffsetFromThumbY(float thumb_y) const;

  void Paint(auralite::Canvas& canvas) const;

  // Returns true if the event was handled (thumb/track drag).
  bool OnMouseDown(const MouseEvent& e);
  bool OnMouseMove(const MouseEvent& e);
  bool OnMouseUp(const MouseEvent& e);
  void CancelDrag();

  bool dragging() const { return dragging_; }

 private:
  float content_h_ = 0.f;
  float viewport_h_ = 0.f;
  float scroll_y_ = 0.f;
  RectF track_{};
  bool dragging_ = false;
  float drag_anchor_y_ = 0.f;
  float drag_scroll_anchor_ = 0.f;
};

}  // namespace auralite::ui
