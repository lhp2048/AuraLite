#include "mx/ui/list_columns.h"

#include "mx/canvas.h"
#include "mx/ui/theme.h"

#include <algorithm>
#include <cmath>

namespace mx::ui {
namespace {

constexpr float kPadX = 8.f;

mx::TextHAlign ToHAlign(TextAlign a) {
  switch (a) {
    case TextAlign::Center:
      return mx::TextHAlign::Center;
    case TextAlign::Right:
      return mx::TextHAlign::Right;
    default:
      return mx::TextHAlign::Left;
  }
}

}  // namespace

std::vector<RectF> ComputeColumnRects(float content_w,
                                      const std::vector<ListColumn>& cols,
                                      float pad_x) {
  std::vector<RectF> out;
  if (cols.empty() || content_w <= 0.f) {
    return out;
  }

  const float inner = std::max(0.f, content_w - pad_x * 2.f);
  float fixed = 0.f;
  float weight_sum = 0.f;
  for (const auto& c : cols) {
    if (c.width > 0.f) {
      fixed += c.width;
    } else {
      weight_sum += std::max(0.f, c.weight);
    }
  }
  const float flex = std::max(0.f, inner - fixed);

  float x = pad_x;
  out.reserve(cols.size());
  for (const auto& c : cols) {
    float w = 0.f;
    if (c.width > 0.f) {
      w = c.width;
    } else if (weight_sum > 0.f) {
      w = flex * (std::max(0.f, c.weight) / weight_sum);
    }
    out.push_back(RectF{x, 0.f, w, 0.f});
    x += w;
  }
  return out;
}

float ColumnsContentWidth(float viewport_w, const std::vector<ListColumn>& cols,
                          float pad_x) {
  if (cols.empty()) {
    return viewport_w;
  }
  const auto rects = ComputeColumnRects(viewport_w, cols, pad_x);
  if (rects.empty()) {
    return viewport_w;
  }
  const float end = rects.back().x + rects.back().w + pad_x;
  // If any weight cols, layout fills viewport; use max(viewport, end).
  bool any_weight = false;
  float fixed_sum = pad_x * 2.f;
  for (const auto& c : cols) {
    if (c.width > 0.f) {
      fixed_sum += c.width;
    } else {
      any_weight = true;
    }
  }
  if (!any_weight) {
    return std::max(viewport_w, fixed_sum);
  }
  return std::max(viewport_w, end);
}

void MaterializeColumnWidths(std::vector<ListColumn>* cols, float content_w,
                             float pad_x) {
  if (!cols || cols->empty()) {
    return;
  }
  const auto rects = ComputeColumnRects(content_w, *cols, pad_x);
  for (size_t i = 0; i < cols->size() && i < rects.size(); ++i) {
    (*cols)[i].width = std::max(1.f, rects[i].w);
    (*cols)[i].weight = 0.f;
  }
}

float FrozenWidth(const std::vector<ListColumn>& cols, float content_w,
                  int frozen_count, float pad_x) {
  if (frozen_count <= 0 || cols.empty()) {
    return 0.f;
  }
  const auto rects = ComputeColumnRects(content_w, cols, pad_x);
  const int n = std::min(frozen_count, static_cast<int>(rects.size()));
  if (n <= 0) {
    return 0.f;
  }
  const float left = rects.front().x;
  const float right = rects[static_cast<size_t>(n - 1)].x +
                      rects[static_cast<size_t>(n - 1)].w;
  return right - left;
}

std::vector<RectF> HeaderColumnCells(const RectF& band,
                                     const std::vector<ListColumn>& cols,
                                     int frozen_count, float scroll_x,
                                     float pad_x) {
  std::vector<RectF> cells;
  const auto base = ComputeColumnRects(band.w, cols, pad_x);
  cells.reserve(base.size());
  const int frozen = std::clamp(frozen_count, 0, static_cast<int>(base.size()));
  for (size_t i = 0; i < base.size(); ++i) {
    float x = band.x + base[i].x;
    if (static_cast<int>(i) >= frozen) {
      x -= scroll_x;
    }
    cells.push_back(RectF{x, band.y, base[i].w, band.h});
  }
  return cells;
}

void PaintColumnDividers(mx::Canvas& canvas, const RectF& band,
                         const std::vector<ListColumn>& cols, int frozen_count,
                         float scroll_x, float pad_x) {
  if (cols.size() < 2 || band.h <= 0.f) {
    return;
  }
  const auto& t = Theme::Active();
  const int frozen =
      std::clamp(frozen_count, 0, static_cast<int>(cols.size()));
  const float fz = FrozenWidth(cols, band.w, frozen, pad_x);
  const auto cells = HeaderColumnCells(band, cols, frozen, scroll_x, pad_x);
  if (cells.size() < 2) {
    return;
  }

  auto draw_range = [&](int from, int to, const RectF& clip) {
    canvas.PushAxisAlignedClip(clip);
    for (int i = from; i < to; ++i) {
      if (frozen > 0 && i == frozen - 1) {
        continue;
      }
      const RectF& c = cells[static_cast<size_t>(i)];
      const float x = c.x + c.w - 0.5f;
      canvas.DrawLine(x, band.y, x, band.y + band.h, t.divider, 1.f);
    }
    canvas.PopAxisAlignedClip();
  };

  if (frozen > 0) {
    draw_range(0, frozen - 1, RectF{band.x, band.y, fz, band.h});
  }
  draw_range(frozen, static_cast<int>(cells.size()) - 1,
             RectF{band.x + fz, band.y, std::max(0.f, band.w - fz), band.h});
}

void PaintListHeader(mx::Canvas& canvas, const RectF& band,
                     const std::vector<ListColumn>& cols, float font_size,
                     const ListHeaderPaintState& state) {
  if (cols.empty() || band.h <= 0.f) {
    return;
  }
  const auto& t = Theme::Active();
  canvas.FillRect(band, t.surface_alt);
  canvas.DrawLine(band.x, band.y + band.h - 0.5f, band.x + band.w,
                  band.y + band.h - 0.5f, t.divider, 1.f);

  const int frozen =
      std::clamp(state.frozen_count, 0, static_cast<int>(cols.size()));
  const float fz = FrozenWidth(cols, band.w, frozen, kPadX);

  // Scrollable header region.
  canvas.PushAxisAlignedClip(
      RectF{band.x + fz, band.y, std::max(0.f, band.w - fz), band.h});
  const auto cells =
      HeaderColumnCells(band, cols, frozen, state.scroll_x, kPadX);
  for (size_t i = static_cast<size_t>(frozen); i < cols.size() && i < cells.size();
       ++i) {
    std::wstring title = cols[i].title;
    if (state.sort_col == static_cast<int>(i) &&
        state.sort_dir != ListSortDir::None) {
      title += (state.sort_dir == ListSortDir::Asc) ? L" ▲" : L" ▼";
    }
    canvas.DrawText(title, cells[i], t.text_muted, font_size, t.font_ui.c_str(),
                    ToHAlign(cols[i].align));
  }
  canvas.PopAxisAlignedClip();

  // Frozen header region on top.
  if (frozen > 0) {
    canvas.PushAxisAlignedClip(RectF{band.x, band.y, fz, band.h});
    canvas.FillRect(RectF{band.x, band.y, fz, band.h}, t.surface_alt);
    for (int i = 0; i < frozen && i < static_cast<int>(cells.size()); ++i) {
      std::wstring title = cols[static_cast<size_t>(i)].title;
      if (state.sort_col == i && state.sort_dir != ListSortDir::None) {
        title += (state.sort_dir == ListSortDir::Asc) ? L" ▲" : L" ▼";
      }
      canvas.DrawText(title, cells[static_cast<size_t>(i)], t.text_muted,
                      font_size, t.font_ui.c_str(),
                      ToHAlign(cols[static_cast<size_t>(i)].align));
    }
    canvas.DrawLine(band.x + fz - 0.5f, band.y, band.x + fz - 0.5f,
                    band.y + band.h, t.divider, 1.f);
    canvas.PopAxisAlignedClip();
  }

  PaintColumnDividers(canvas, band, cols, frozen, state.scroll_x, kPadX);
}

int HitHeaderColumn(float x, float y, const RectF& band,
                    const std::vector<ListColumn>& cols, int frozen_count,
                    float scroll_x, float pad_x) {
  if (x < band.x || y < band.y || x >= band.x + band.w || y >= band.y + band.h) {
    return -1;
  }
  const auto cells =
      HeaderColumnCells(band, cols, frozen_count, scroll_x, pad_x);
  const int frozen =
      std::clamp(frozen_count, 0, static_cast<int>(cols.size()));
  const float fz = FrozenWidth(cols, band.w, frozen, pad_x);
  for (int i = static_cast<int>(cells.size()) - 1; i >= 0; --i) {
    const RectF& c = cells[static_cast<size_t>(i)];
    if (i >= frozen && x < band.x + fz) {
      continue;
    }
    if (x >= c.x && x < c.x + c.w && y >= c.y && y < c.y + c.h) {
      return i;
    }
  }
  return -1;
}

int HitHeaderSplitter(float x, float y, const RectF& band,
                      const std::vector<ListColumn>& cols, int frozen_count,
                      float scroll_x, float pad_x, float hit_slop) {
  if (x < band.x || y < band.y || x >= band.x + band.w || y >= band.y + band.h) {
    return -1;
  }
  const auto cells =
      HeaderColumnCells(band, cols, frozen_count, scroll_x, pad_x);
  for (int i = 0; i < static_cast<int>(cells.size()); ++i) {
    if (!cols[static_cast<size_t>(i)].resizable) {
      continue;
    }
    const float edge = cells[static_cast<size_t>(i)].x +
                       cells[static_cast<size_t>(i)].w;
    if (std::fabs(x - edge) <= hit_slop) {
      return i;
    }
  }
  return -1;
}

}  // namespace mx::ui
