#pragma once

#include "auralite/ui/node.h"

#include <memory>

namespace auralite::ui {

// Vertical-only scroll container. Single content child; clips to viewport.
class ScrollView : public Node {
 public:
  ScrollView& preferred_size(float w, float h);
  ScrollView& set_content(std::unique_ptr<Node> content);
  Node* content() const;

  void set_scroll_offset(float offset);
  float scroll_offset() const { return scroll_offset_; }

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;
  void Paint(auralite::Canvas& canvas) override;
  Node* HitTest(float x, float y) override;

  bool WantsMouseWheel() const override { return true; }
  void OnMouseWheel(const MouseEvent& e) override;
  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;

 private:
  static constexpr float kScrollbarWidth = 10.f;
  static constexpr float kMinThumbHeight = 20.f;
  static constexpr float kDefaultLineScroll = 40.f;

  float ContentHeight() const;
  float ViewportWidth() const;
  float ViewportHeight() const;
  float MaxScrollOffset() const;
  bool NeedsScrollbar() const;
  void ClampScrollOffset();
  RectF ViewportRect() const;
  RectF ScrollbarBounds() const;
  RectF ThumbBounds() const;
  float ScrollOffsetFromThumbY(float thumb_y) const;

  float preferred_w_ = 0.f;
  float preferred_h_ = 0.f;
  float scroll_offset_ = 0.f;
  float content_h_ = 0.f;
  bool dragging_thumb_ = false;
  float drag_thumb_anchor_y_ = 0.f;
  float drag_scroll_anchor_ = 0.f;
};

}  // namespace auralite::ui
