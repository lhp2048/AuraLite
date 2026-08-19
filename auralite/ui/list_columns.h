#pragma once

#include "auralite/ui/types.h"

#include <functional>
#include <string>
#include <vector>

namespace auralite {
class Canvas;
}

namespace auralite::ui {

enum class ColumnSortKind {
  Auto,     // column-wide numeric when all cells parse as numbers, else natural
  Text,     // lexicographic wstring
  Number,   // numeric; non-numeric sorts after numbers
  Natural,  // text with numeric chunks ("行 2" before "行 10")
};

struct ListColumn {
  std::wstring title;
  float width = 0.f;   // >0: fixed px; else use weight
  float weight = 1.f;
  TextAlign align = TextAlign::Left;
  bool sortable = true;
  bool resizable = true;
  bool editable = true;  // DataGrid cell edit; ignored by plain VirtualList
  ColumnSortKind sort_kind = ColumnSortKind::Auto;  // DataGrid sort; ignored by VirtualList
};

enum class ListSortDir { None = 0, Asc = 1, Desc = -1 };

// Split |content_w| into column rects with origin x=0 (caller offsets).
std::vector<RectF> ComputeColumnRects(float content_w,
                                      const std::vector<ListColumn>& cols,
                                      float pad_x = 8.f);

// Sum of laid-out column widths + horizontal padding.
float ColumnsContentWidth(float viewport_w, const std::vector<ListColumn>& cols,
                          float pad_x = 8.f);

// Turn weight columns into fixed widths using current layout.
void MaterializeColumnWidths(std::vector<ListColumn>* cols, float content_w,
                             float pad_x = 8.f);

struct ListHeaderPaintState {
  int sort_col = -1;
  ListSortDir sort_dir = ListSortDir::None;
  int frozen_count = 0;
  float scroll_x = 0.f;
};

void PaintListHeader(auralite::Canvas& canvas, const RectF& band,
                     const std::vector<ListColumn>& cols, float font_size,
                     const ListHeaderPaintState& state = {});

// Vertical dividers between columns (skips frozen | scroll seam).
void PaintColumnDividers(auralite::Canvas& canvas, const RectF& band,
                         const std::vector<ListColumn>& cols, int frozen_count,
                         float scroll_x, float pad_x = 8.f);

// Absolute column cell rects in band coordinates (applies freeze + scroll_x).
std::vector<RectF> HeaderColumnCells(const RectF& band,
                                     const std::vector<ListColumn>& cols,
                                     int frozen_count, float scroll_x,
                                     float pad_x = 8.f);

int HitHeaderColumn(float x, float y, const RectF& band,
                    const std::vector<ListColumn>& cols, int frozen_count,
                    float scroll_x, float pad_x = 8.f);

// Returns column index whose right edge is near x, or -1.
int HitHeaderSplitter(float x, float y, const RectF& band,
                      const std::vector<ListColumn>& cols, int frozen_count,
                      float scroll_x, float pad_x = 8.f,
                      float hit_slop = 5.f);

float FrozenWidth(const std::vector<ListColumn>& cols, float content_w,
                  int frozen_count, float pad_x = 8.f);

}  // namespace auralite::ui
