#pragma once

#include "mx/ui/anim.h"
#include "mx/ui/node.h"
#include "mx/ui/vertical_scrollbar.h"

#include <memory>

namespace mx::ui {

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
  void Paint(mx::Canvas& canvas) override;
  Node* HitTest(float x, float y) override;

  bool WantsMouseWheel() const override { return true; }
  void OnMouseWheel(const MouseEvent& e) override;
  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;

 private:
  static constexpr float kDefaultLineScroll = 40.f;

  void SyncVScrollBar();
  float ContentHeight() const;
  float ViewportWidth() const;
  float ViewportHeight() const;
  float MaxScrollOffset() const;
  bool NeedsScrollbar() const;
  void ClampScrollOffset();
  RectF ViewportRect() const;
  void RelayoutContent();
  void ApplyScroll(float offset, bool instant);
  void OnAnimateChanged() override;
  void OnHostWindowChanged() override;

  float scroll_offset_ = 0.f;
  float scroll_target_ = 0.f;
  float content_h_ = 0.f;
  Tween scroll_tween_;
  VerticalScrollbar vscroll_;
};

}  // namespace mx::ui
