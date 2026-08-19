#include "mx/ui/tile.h"

#include <algorithm>

namespace mx::ui {

Tile::Tile() {
  fill_width();
  hug_height();
  clip_children(true);
}

Tile& Tile::padding(float all) {
  pad_l_ = pad_t_ = pad_r_ = pad_b_ = all;
  return *this;
}

Tile& Tile::padding(float left, float top, float right, float bottom) {
  pad_l_ = left;
  pad_t_ = top;
  pad_r_ = right;
  pad_b_ = bottom;
  return *this;
}

Tile& Tile::spacing(float s) {
  spacing_x_ = spacing_y_ = s;
  return *this;
}

Tile& Tile::spacing(float horizontal, float vertical) {
  spacing_x_ = horizontal;
  spacing_y_ = vertical;
  return *this;
}

Tile& Tile::columns(int cols) {
  columns_ = std::max(0, cols);
  return *this;
}

Tile& Tile::item_size(float w, float h) {
  item_w_ = std::max(1.f, w);
  item_h_ = std::max(1.f, h);
  return *this;
}

void Tile::ComputeGrid(float inner_w, int child_count, int* out_cols,
                       float* out_cell_w, float* out_cell_h) const {
  int cols = columns_;
  float cell_w = item_w_;
  float cell_h = item_h_;

  if (cols <= 0) {
    const float pitch = item_w_ + spacing_x_;
    cols = 1;
    if (pitch > 0.f && inner_w >= item_w_) {
      cols = std::max(
          1, static_cast<int>((inner_w + spacing_x_) / pitch));
    }
  } else {
    const float gaps = spacing_x_ * static_cast<float>(cols - 1);
    cell_w = std::max(1.f, (inner_w - gaps) / static_cast<float>(cols));
    if (item_h_ > 0.f) {
      cell_h = item_h_;
    } else {
      cell_h = cell_w;
    }
  }

  if (child_count <= 0) {
    cols = std::max(1, cols);
  }
  *out_cols = std::max(1, cols);
  *out_cell_w = cell_w;
  *out_cell_h = cell_h;
}

SizeF Tile::Measure(float max_w, float max_h) {
  const float inner_w = std::max(0.f, max_w - pad_l_ - pad_r_);
  int n = 0;
  for (const auto& c : children_) {
    if (c) {
      ++n;
    }
  }

  int cols = 1;
  float cell_w = item_w_;
  float cell_h = item_h_;
  ComputeGrid(inner_w, n, &cols, &cell_w, &cell_h);

  const int rows = n > 0 ? (n + cols - 1) / cols : 0;
  const float hug_w =
      pad_l_ + pad_r_ +
      (cols > 0 ? (cell_w * cols + spacing_x_ * (cols - 1)) : 0.f);
  const float hug_h =
      pad_t_ + pad_b_ +
      (rows > 0 ? (cell_h * rows + spacing_y_ * (rows - 1)) : 0.f);
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void Tile::Layout(const RectF& final_rect) {
  bounds_ = final_rect;
  const float inner_x = final_rect.x + pad_l_;
  const float inner_y = final_rect.y + pad_t_;
  const float inner_w = std::max(0.f, final_rect.w - pad_l_ - pad_r_);

  int n = 0;
  for (const auto& c : children_) {
    if (c) {
      ++n;
    }
  }

  int cols = 1;
  float cell_w = item_w_;
  float cell_h = item_h_;
  ComputeGrid(inner_w, n, &cols, &cell_w, &cell_h);

  int index = 0;
  for (auto& child : children_) {
    if (!child) {
      continue;
    }
    const int row = index / cols;
    const int col = index % cols;
    const float x = inner_x + static_cast<float>(col) * (cell_w + spacing_x_);
    const float y = inner_y + static_cast<float>(row) * (cell_h + spacing_y_);
    child->Layout(RectF{x, y, cell_w, cell_h});
    ++index;
  }
}

}  // namespace mx::ui
