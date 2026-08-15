#pragma once

#include "auralite/ui/node.h"

#include <memory>

namespace auralite::ui {

// Horizontal split: leading (left) | divider | trailing (right).
// Drag the divider to resize. Ratio is leading_width / (total - divider).
class SplitView : public Node {
 public:
  SplitView& preferred_size(float w, float h);
  SplitView& set_leading(std::unique_ptr<Node> leading);
  SplitView& set_trailing(std::unique_ptr<Node> trailing);
  SplitView& set_ratio(float ratio);  // 0..1, default 0.5

  Node* leading() const;
  Node* trailing() const;
  float ratio() const { return ratio_; }

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;
  void Paint(auralite::Canvas& canvas) override;
  Node* HitTest(float x, float y) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;

 private:
  static constexpr float kDividerSize = 5.f;
  static constexpr float kMinPane = 40.f;

  RectF DividerBounds() const;
  bool IsPointInDivider(float x, float y) const;
  void ApplyRatioFromDividerOffset(float offset);
  void RelayoutChildren();

  float preferred_w_ = 0.f;
  float preferred_h_ = 0.f;
  float ratio_ = 0.5f;
  bool dragging_ = false;
};

}  // namespace auralite::ui
