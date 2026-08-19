#pragma once

#include "mx/ui/node.h"

namespace mx::ui {

// Grid / toolbox layout (DuiLib TileLayout).
// Prefer columns>0 (fixed column count) OR item_size (cell WxH, wrap by width).
class Tile : public Node {
 public:
  Tile();

  Tile& padding(float all);
  Tile& padding(float left, float top, float right, float bottom);
  Tile& spacing(float s);
  Tile& spacing(float horizontal, float vertical);
  // Fixed column count; 0 = derive from item_size + available width.
  Tile& columns(int cols);
  // Cell size when columns==0; also used as default cell height when columns>0.
  Tile& item_size(float w, float h);

  int columns() const { return columns_; }

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;

 private:
  void ComputeGrid(float inner_w, int child_count, int* out_cols,
                   float* out_cell_w, float* out_cell_h) const;

  float pad_l_ = 0.f;
  float pad_t_ = 0.f;
  float pad_r_ = 0.f;
  float pad_b_ = 0.f;
  float spacing_x_ = 8.f;
  float spacing_y_ = 8.f;
  int columns_ = 0;
  float item_w_ = 80.f;
  float item_h_ = 80.f;
};

}  // namespace mx::ui
