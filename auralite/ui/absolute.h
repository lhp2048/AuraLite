#pragma once

#include "auralite/ui/node.h"

namespace auralite::ui {

// Absolute / float host (DuiLib float style).
// Children: edge anchors (left/top/right/bottom) preferred; else set_pos + size.
class Absolute : public Node {
 public:
  Absolute();

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;
};

}  // namespace auralite::ui
