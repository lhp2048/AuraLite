#pragma once

#include "mx/ui/node.h"

namespace mx::ui {

// Per axis: dual-edge > single-edge > x/y > h_align / v_align > origin 0.
// Dual-edge sets that axis' size and ignores width/height (and control defaults).
// No compile-time conflict if both are written; anchors win.
// Free axis: Node::h_align / v_align Start/Center/End (e.g. right + v_align center).
class Absolute : public Node {
 public:
  Absolute();

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;
};

}  // namespace mx::ui
