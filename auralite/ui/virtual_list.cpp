#include "auralite/ui/virtual_list.h"

#include "auralite/ui/theme.h"

#include <algorithm>
#include <cmath>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace auralite::ui {
namespace {

int KindIndex(VirtualListItemKind k) {
  return static_cast<int>(k);
}

}  // namespace

VirtualList::VirtualList() {
  set_focusable(true);
  fill_width();
  fixed_height(200.f);
}

AccRole VirtualList::acc_role() const {
  if (acc_role_override_) {
    return *acc_role_override_;
  }
  return AccRole::List;
}

VirtualList& VirtualList::item_count(ItemCountFn fn) {
  count_fn_ = std::move(fn);
  InvalidateData();
  return *this;
}

VirtualList& VirtualList::item_kind(ItemKindFn fn) {
  kind_fn_ = std::move(fn);
  InvalidateData();
  return *this;
}

VirtualList& VirtualList::item_text(ItemTextFn fn) {
  text_fn_ = std::move(fn);
  return *this;
}

VirtualList& VirtualList::item_sub_text(ItemSubTextFn fn) {
  sub_text_fn_ = std::move(fn);
  return *this;
}

VirtualList& VirtualList::item_cell_text(ItemCellTextFn fn) {
  cell_text_fn_ = std::move(fn);
  return *this;
}

VirtualList& VirtualList::item_checked(ItemCheckedFn fn) {
  checked_fn_ = std::move(fn);
  return *this;
}

VirtualList& VirtualList::item_set_checked(ItemCheckSetFn fn) {
  set_checked_fn_ = std::move(fn);
  return *this;
}

VirtualList& VirtualList::on_paint_item(PaintItemFn fn) {
  paint_fn_ = std::move(fn);
  return *this;
}

VirtualList& VirtualList::on_selection_changed(SelectionHandler handler) {
  on_selection_ = std::move(handler);
  return *this;
}

VirtualList& VirtualList::on_check_changed(CheckHandler handler) {
  on_check_ = std::move(handler);
  return *this;
}

VirtualList& VirtualList::row_height(VirtualListItemKind kind, float h) {
  heights_[KindIndex(kind)] = std::max(1.f, h);
  InvalidateData();
  return *this;
}

float VirtualList::row_height(VirtualListItemKind kind) const {
  return heights_[KindIndex(kind)];
}

VirtualList& VirtualList::font_size(float size) {
  font_size_ = size;
  return *this;
}

VirtualList& VirtualList::overscan(int rows) {
  overscan_ = std::max(0, rows);
  return *this;
}

VirtualList& VirtualList::columns(std::vector<ListColumn> cols) {
  columns_ = std::move(cols);
  ClampScrollX();
  return *this;
}

VirtualList& VirtualList::show_header(bool v) {
  show_header_ = v;
  ClampScroll();
  return *this;
}

VirtualList& VirtualList::header_height(float h) {
  header_h_ = std::max(0.f, h);
  ClampScroll();
  return *this;
}

VirtualList& VirtualList::frozen_count(int n) {
  frozen_count_ = std::max(0, n);
  ClampScrollX();
  return *this;
}

VirtualList& VirtualList::on_sort_changed(SortHandler handler) {
  on_sort_ = std::move(handler);
  return *this;
}

void VirtualList::set_sort(int col, ListSortDir dir, bool notify) {
  if (col < -1 || col >= static_cast<int>(columns_.size())) {
    return;
  }
  sort_col_ = col;
  sort_dir_ = (col < 0) ? ListSortDir::None : dir;
  if (notify && on_sort_) {
    on_sort_(sort_col_, sort_dir_);
  }
}

std::vector<RectF> VirtualList::ColumnRects() const {
  return HeaderColumnCells(
      RectF{bounds_.x, BodyTop(), ViewportWidth(),
            row_height(VirtualListItemKind::Text)},
      columns_, frozen_count_, scroll_x_);
}

float VirtualList::HeaderBand() const {
  return (show_header_ && !columns_.empty()) ? header_h_ : 0.f;
}

float VirtualList::BodyTop() const { return bounds_.y + HeaderBand(); }

RectF VirtualList::HeaderBandRect() const {
  return RectF{bounds_.x, bounds_.y, ViewportWidth(), HeaderBand()};
}

ListHeaderPaintState VirtualList::MakeHeaderPaintState() const {
  ListHeaderPaintState st;
  st.sort_col = sort_col_;
  st.sort_dir = sort_dir_;
  st.frozen_count = frozen_count_;
  st.scroll_x = scroll_x_;
  return st;
}

float VirtualList::MaxScrollX() const {
  return std::max(0.f,
                  ColumnsContentWidth(ViewportWidth(), columns_) - ViewportWidth());
}

void VirtualList::ClampScrollX() {
  scroll_x_ = std::clamp(scroll_x_, 0.f, MaxScrollX());
}

void VirtualList::CycleSort(int col) {
  if (col < 0 || col >= static_cast<int>(columns_.size()) ||
      !columns_[static_cast<size_t>(col)].sortable) {
    return;
  }
  if (sort_col_ != col) {
    sort_col_ = col;
    sort_dir_ = ListSortDir::Asc;
  } else if (sort_dir_ == ListSortDir::Asc) {
    sort_dir_ = ListSortDir::Desc;
  } else if (sort_dir_ == ListSortDir::Desc) {
    sort_col_ = -1;
    sort_dir_ = ListSortDir::None;
  } else {
    sort_dir_ = ListSortDir::Asc;
  }
  if (on_sort_) {
    on_sort_(sort_col_, sort_dir_);
  }
  Invalidate();
}

bool VirtualList::HandleHeaderMouseDown(const MouseEvent& e) {
  if (HeaderBand() <= 0.f) {
    return false;
  }
  const RectF band = HeaderBandRect();
  if (e.y < band.y || e.y >= band.y + band.h || e.x < band.x ||
      e.x >= band.x + band.w) {
    return false;
  }
  const int split =
      HitHeaderSplitter(e.x, e.y, band, columns_, frozen_count_, scroll_x_);
  if (split >= 0) {
    MaterializeColumnWidths(&columns_, ViewportWidth());
    resizing_col_ = true;
    resize_col_ = split;
    resize_anchor_x_ = e.x;
    resize_anchor_w_ = columns_[static_cast<size_t>(split)].width;
    return true;
  }
  const int col =
      HitHeaderColumn(e.x, e.y, band, columns_, frozen_count_, scroll_x_);
  if (col >= 0) {
    CycleSort(col);
  }
  return true;
}

bool VirtualList::HandleHeaderMouseMove(const MouseEvent& e) {
  if (!resizing_col_ || resize_col_ < 0 ||
      resize_col_ >= static_cast<int>(columns_.size())) {
    return false;
  }
  const float nw =
      std::max(min_col_w_, resize_anchor_w_ + (e.x - resize_anchor_x_));
  columns_[static_cast<size_t>(resize_col_)].width = nw;
  columns_[static_cast<size_t>(resize_col_)].weight = 0.f;
  ClampScrollX();
  return true;
}

void VirtualList::InvalidateData() {
  prefix_dirty_ = true;
  if (selected_index_ >= Count()) {
    selected_index_ = Count() > 0 ? Count() - 1 : -1;
  }
  ClampScroll();
}

int VirtualList::Count() const {
  if (!count_fn_) {
    return 0;
  }
  return std::max(0, count_fn_());
}

VirtualListItemKind VirtualList::KindAt(int index) const {
  if (kind_fn_) {
    return kind_fn_(index);
  }
  return VirtualListItemKind::Text;
}

std::wstring VirtualList::TextAt(int index) const {
  return text_fn_ ? text_fn_(index) : L"";
}

std::wstring VirtualList::SubTextAt(int index) const {
  return sub_text_fn_ ? sub_text_fn_(index) : L"";
}

std::wstring VirtualList::CellTextAt(int index, int col) const {
  if (cell_text_fn_) {
    return cell_text_fn_(index, col);
  }
  if (col == 0) {
    return TextAt(index);
  }
  return L"";
}

bool VirtualList::CheckedAt(int index) const {
  return checked_fn_ ? checked_fn_(index) : false;
}

void VirtualList::SetCheckedAt(int index, bool checked) {
  if (set_checked_fn_) {
    set_checked_fn_(index, checked);
  }
  if (on_check_) {
    on_check_(index, checked);
  }
}

void VirtualList::RebuildPrefixIfNeeded() {
  if (!prefix_dirty_) {
    return;
  }
  const int n = Count();
  prefix_y_.assign(static_cast<size_t>(n) + 1, 0.f);
  for (int i = 0; i < n; ++i) {
    prefix_y_[static_cast<size_t>(i) + 1] =
        prefix_y_[static_cast<size_t>(i)] + RowHeightAt(i);
  }
  content_h_ = prefix_y_.back();
  prefix_dirty_ = false;
}

float VirtualList::ContentHeight() const {
  const_cast<VirtualList*>(this)->RebuildPrefixIfNeeded();
  return content_h_;
}

float VirtualList::RowTop(int index) const {
  const_cast<VirtualList*>(this)->RebuildPrefixIfNeeded();
  if (index < 0 || index >= static_cast<int>(prefix_y_.size()) - 1) {
    return 0.f;
  }
  return prefix_y_[static_cast<size_t>(index)];
}

float VirtualList::RowHeightAt(int index) const {
  return row_height(KindAt(index));
}

float VirtualList::ViewportWidth() const {
  return NeedsScrollbar()
             ? std::max(0.f, bounds_.w - VerticalScrollbar::kWidth)
             : bounds_.w;
}

float VirtualList::ViewportHeight() const {
  float h = std::max(0.f, bounds_.h - HeaderBand());
  if (NeedsHScrollbar()) {
    h = std::max(0.f, h - HorizontalScrollbar::kHeight);
  }
  return h;
}

float VirtualList::MaxScrollOffset() const {
  return std::max(0.f, ContentHeight() - ViewportHeight());
}

void VirtualList::ResolveScrollNeeds(bool* need_v, bool* need_h) const {
  const float body_w = bounds_.w > 0.f
                           ? bounds_.w
                           : (preferred_width() > 0.f ? preferred_width() : 0.f);
  const float total_h = bounds_.h > 0.f
                            ? bounds_.h
                            : (preferred_height() > 0.f ? preferred_height() : 0.f);
  const float body_h = std::max(0.f, total_h - HeaderBand());

  *need_v = ContentHeight() > body_h && body_h > 0.f;
  float vw =
      *need_v ? std::max(0.f, body_w - VerticalScrollbar::kWidth) : body_w;
  *need_h = !columns_.empty() && vw > 0.f &&
            ColumnsContentWidth(vw, columns_) > vw;
  if (*need_h) {
    const float vh = std::max(0.f, body_h - HorizontalScrollbar::kHeight);
    *need_v = ContentHeight() > vh && vh > 0.f;
    vw = *need_v ? std::max(0.f, body_w - VerticalScrollbar::kWidth) : body_w;
    *need_h = !columns_.empty() && vw > 0.f &&
              ColumnsContentWidth(vw, columns_) > vw;
  }
}

bool VirtualList::NeedsScrollbar() const {
  bool need_v = false;
  bool need_h = false;
  ResolveScrollNeeds(&need_v, &need_h);
  return need_v;
}

bool VirtualList::NeedsHScrollbar() const {
  bool need_v = false;
  bool need_h = false;
  ResolveScrollNeeds(&need_v, &need_h);
  return need_h;
}

void VirtualList::ClampScroll() {
  scroll_y_ = std::clamp(scroll_y_, 0.f, MaxScrollOffset());
}

void VirtualList::set_scroll_offset(float y) {
  scroll_y_ = y;
  ClampScroll();
}

void VirtualList::SyncVScrollBar() {
  vscroll_.set_content_height(ContentHeight());
  vscroll_.set_viewport_height(ViewportHeight());
  vscroll_.set_scroll_offset(scroll_y_);
  vscroll_.set_track_bounds(
      RectF{bounds_.x + bounds_.w - VerticalScrollbar::kWidth, BodyTop(),
            VerticalScrollbar::kWidth, ViewportHeight()});
  scroll_y_ = vscroll_.scroll_offset();
}

void VirtualList::SyncHScrollBar() {
  const float vw = ViewportWidth();
  hscroll_.set_content_width(ColumnsContentWidth(vw, columns_));
  hscroll_.set_viewport_width(vw);
  hscroll_.set_scroll_offset(scroll_x_);
  hscroll_.set_track_bounds(
      RectF{bounds_.x, bounds_.y + bounds_.h - HorizontalScrollbar::kHeight, vw,
            HorizontalScrollbar::kHeight});
  scroll_x_ = hscroll_.scroll_offset();
}

void VirtualList::VisibleRange(int* first, int* last) const {
  const int n = Count();
  if (n <= 0) {
    *first = 0;
    *last = -1;
    return;
  }
  const_cast<VirtualList*>(this)->RebuildPrefixIfNeeded();
  const float y0 = scroll_y_;
  const float y1 = scroll_y_ + ViewportHeight();

  int lo = 0;
  int hi = n;
  while (lo < hi) {
    const int mid = (lo + hi) / 2;
    if (prefix_y_[static_cast<size_t>(mid) + 1] <= y0) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  *first = std::max(0, lo - overscan_);

  lo = *first;
  hi = n;
  while (lo < hi) {
    const int mid = (lo + hi) / 2;
    if (prefix_y_[static_cast<size_t>(mid)] < y1) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  *last = std::min(n - 1, lo - 1 + overscan_);
}

int VirtualList::IndexAtContentY(float content_y) const {
  const int n = Count();
  if (n <= 0) {
    return -1;
  }
  const_cast<VirtualList*>(this)->RebuildPrefixIfNeeded();
  if (content_y < 0.f || content_y >= content_h_) {
    return -1;
  }
  int lo = 0;
  int hi = n - 1;
  while (lo <= hi) {
    const int mid = (lo + hi) / 2;
    const float top = prefix_y_[static_cast<size_t>(mid)];
    const float bottom = prefix_y_[static_cast<size_t>(mid) + 1];
    if (content_y < top) {
      hi = mid - 1;
    } else if (content_y >= bottom) {
      lo = mid + 1;
    } else {
      return mid;
    }
  }
  return -1;
}

int VirtualList::IndexAtPoint(float x, float y) const {
  const RectF vp{bounds_.x, BodyTop(), ViewportWidth(), ViewportHeight()};
  if (!ContainsPoint(vp, x, y)) {
    return -1;
  }
  return IndexAtContentY((y - BodyTop()) + scroll_y_);
}

RectF VirtualList::RowRect(int index) const {
  const float top = RowTop(index) - scroll_y_;
  return RectF{bounds_.x, BodyTop() + top, ViewportWidth(), RowHeightAt(index)};
}

RectF VirtualList::CheckBoxRect(const RectF& row) const {
  const float box = std::min(16.f, row.h - 8.f);
  return RectF{row.x + kPadX, row.y + (row.h - box) * 0.5f, box, box};
}

bool VirtualList::HitCheckBox(int index, float x, float y) const {
  if (KindAt(index) != VirtualListItemKind::Checkable) {
    return false;
  }
  return ContainsPoint(CheckBoxRect(RowRect(index)), x, y);
}

bool VirtualList::HitCellAtPoint(float x, float y, int* row, int* col) const {
  const int r = IndexAtPoint(x, y);
  if (r < 0 || columns_.empty()) {
    return false;
  }
  const RectF row_r = RowRect(r);
  const int frozen =
      std::clamp(frozen_count_, 0, static_cast<int>(columns_.size()));
  const auto cells =
      HeaderColumnCells(row_r, columns_, frozen, scroll_x_, kPadX);
  for (int c = 0; c < static_cast<int>(cells.size()); ++c) {
    if (ContainsPoint(cells[static_cast<size_t>(c)], x, y)) {
      if (row) {
        *row = r;
      }
      if (col) {
        *col = c;
      }
      return true;
    }
  }
  return false;
}

RectF VirtualList::CellRectAt(int row, int col) const {
  if (row < 0 || col < 0 || columns_.empty()) {
    return {};
  }
  const RectF row_r = RowRect(row);
  const int frozen =
      std::clamp(frozen_count_, 0, static_cast<int>(columns_.size()));
  const auto cells =
      HeaderColumnCells(row_r, columns_, frozen, scroll_x_, kPadX);
  if (col >= static_cast<int>(cells.size())) {
    return {};
  }
  return cells[static_cast<size_t>(col)];
}

void VirtualList::CommitSelection() {
  if (on_selection_ && selected_index_ >= 0) {
    on_selection_(selected_index_);
  }
}

void VirtualList::set_selected_index(int index, bool notify) {
  const int n = Count();
  if (index < -1 || index >= n) {
    return;
  }
  if (selected_index_ == index) {
    if (notify) {
      CommitSelection();
    }
    return;
  }
  selected_index_ = index;
  if (selected_index_ >= 0) {
    EnsureVisible(selected_index_);
  }
  if (notify) {
    CommitSelection();
  }
}

void VirtualList::EnsureVisible(int index) {
  if (index < 0 || index >= Count()) {
    return;
  }
  RebuildPrefixIfNeeded();
  const float top = RowTop(index);
  const float bottom = top + RowHeightAt(index);
  const float view_h = ViewportHeight();
  if (top < scroll_y_) {
    set_scroll_offset(top);
  } else if (bottom > scroll_y_ + view_h) {
    set_scroll_offset(bottom - view_h);
  }
}

bool VirtualList::HeaderResizeCursorAt(float x, float y) const {
  if (HeaderBand() <= 0.f || columns_.empty()) {
    return false;
  }
  return HitHeaderSplitter(x, y, HeaderBandRect(), columns_, frozen_count_,
                           scroll_x_) >= 0;
}

SizeF VirtualList::Measure(float max_w, float max_h) {
  const float hug_h =
      preferred_height() > 0.f ? preferred_height() : 200.f;
  return ResolveSize(max_w, max_h,
                     preferred_width() > 0.f ? preferred_width() : max_w, hug_h);
}

void VirtualList::Layout(const RectF& final_rect) {
  bounds_ = final_rect;
  ClampScroll();
}

void VirtualList::PaintDefaultRow(auralite::Canvas& canvas, const RectF& row,
                                  VirtualListItemKind kind,
                                  const VirtualListItemState& state) {
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  const wchar_t* font = th.font_ui.c_str();
  const ColorF bg = state.selected ? th.accent
                    : state.hovered ? th.accent_soft
                                    : th.surface;
  if (state.selected || state.hovered) {
    canvas.FillRect(row, bg);
  }
  const ColorF text_c = state.selected ? th.text_on_accent : th.text;
  const ColorF sub_c = state.selected ? th.text_on_accent : th.text_muted;

  float text_x = row.x + kPadX;
  const float text_w_right = row.x + row.w - kPadX;

  // Multi-column Text rows when columns_ configured.
  if (kind == VirtualListItemKind::Text && !columns_.empty()) {
    const int frozen =
        std::clamp(frozen_count_, 0, static_cast<int>(columns_.size()));
    const float fz = FrozenWidth(columns_, row.w, frozen, kPadX);
    const auto cells =
        HeaderColumnCells(row, columns_, frozen, scroll_x_, kPadX);

    auto draw_cell = [&](int c) {
      std::wstring t = CellTextAt(state.index, c);
      auralite::TextHAlign ha = auralite::TextHAlign::Left;
      if (columns_[static_cast<size_t>(c)].align == TextAlign::Center) {
        ha = auralite::TextHAlign::Center;
      } else if (columns_[static_cast<size_t>(c)].align == TextAlign::Right) {
        ha = auralite::TextHAlign::Right;
      }
      canvas.DrawText(t, cells[static_cast<size_t>(c)], text_c, fs, font, ha);
    };

    canvas.PushAxisAlignedClip(
        RectF{row.x + fz, row.y, std::max(0.f, row.w - fz), row.h});
    for (int c = frozen; c < static_cast<int>(columns_.size()); ++c) {
      draw_cell(c);
    }
    canvas.PopAxisAlignedClip();

    if (frozen > 0) {
      canvas.PushAxisAlignedClip(RectF{row.x, row.y, fz, row.h});
      if (state.selected) {
        canvas.FillRect(RectF{row.x, row.y, fz, row.h}, th.accent);
      } else if (state.hovered) {
        canvas.FillRect(RectF{row.x, row.y, fz, row.h}, th.accent_soft);
      } else {
        canvas.FillRect(RectF{row.x, row.y, fz, row.h}, th.surface);
      }
      for (int c = 0; c < frozen; ++c) {
        draw_cell(c);
      }
      canvas.DrawLine(row.x + fz - 0.5f, row.y, row.x + fz - 0.5f, row.y + row.h,
                      th.divider, 1.f);
      canvas.PopAxisAlignedClip();
    }

    PaintColumnDividers(canvas, row, columns_, frozen, scroll_x_, kPadX);

    if (state.focused_row && focused()) {
      canvas.DrawRect(row, th.border_focus, 1.5f);
    }
    return;
  }

  if (kind == VirtualListItemKind::Checkable) {
    const RectF box = CheckBoxRect(row);
    canvas.FillRect(box, th.surface);
    canvas.DrawRect(box, state.selected ? th.border_focus : th.border, 1.f);
    if (state.checked) {
      canvas.DrawLine(box.x + 3.f, box.y + box.h * 0.55f, box.x + box.w * 0.4f,
                      box.y + box.h - 3.f, th.glyph, 1.8f);
      canvas.DrawLine(box.x + box.w * 0.4f, box.y + box.h - 3.f,
                      box.x + box.w - 3.f, box.y + 3.f, th.glyph, 1.8f);
    }
    text_x = box.x + box.w + 8.f;
  } else if (kind == VirtualListItemKind::IconText) {
    const float icon = std::min(22.f, row.h - 10.f);
    const RectF icon_r{row.x + kPadX, row.y + (row.h - icon) * 0.5f, icon, icon};
    canvas.FillRoundedRect(icon_r, 4.f, 4.f, th.accent);
    text_x = icon_r.x + icon + 8.f;
  }

  const std::wstring title = TextAt(state.index);
  if (kind == VirtualListItemKind::TwoLine) {
    const float half = row.h * 0.5f;
    canvas.DrawText(title, RectF{text_x, row.y + 2.f,
                                 std::max(0.f, text_w_right - text_x), half},
                    text_c, fs, font, auralite::TextHAlign::Left);
    canvas.DrawText(SubTextAt(state.index),
                    RectF{text_x, row.y + half - 2.f,
                          std::max(0.f, text_w_right - text_x), half},
                    sub_c, th.font_size_sm, font, auralite::TextHAlign::Left);
  } else if (kind == VirtualListItemKind::Custom) {
    canvas.DrawText(title.empty() ? L"（Custom）" : title,
                    RectF{text_x, row.y, std::max(0.f, text_w_right - text_x),
                          row.h},
                    text_c, fs, font, auralite::TextHAlign::Left);
  } else {
    canvas.DrawText(title,
                    RectF{text_x, row.y, std::max(0.f, text_w_right - text_x),
                          row.h},
                    text_c, fs, font, auralite::TextHAlign::Left);
  }

  if (state.focused_row && focused()) {
    canvas.DrawRect(row, th.border_focus, 1.5f);
  }
}

void VirtualList::Paint(auralite::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  RebuildPrefixIfNeeded();
  const ThemeTokens& th = Theme::Active();
  canvas.FillRect(bounds_, th.surface);
  canvas.DrawRect(bounds_, focused() ? th.border_focus : th.border,
                  focused() ? 1.5f : 1.f);

  if (HeaderBand() > 0.f) {
    PaintListHeader(canvas, HeaderBandRect(), columns_, th.font_size_sm,
                    MakeHeaderPaintState());
  }

  const RectF clip{bounds_.x, BodyTop(), ViewportWidth(), ViewportHeight()};
  canvas.PushAxisAlignedClip(clip);

  int first = 0;
  int last = -1;
  VisibleRange(&first, &last);
  for (int i = first; i <= last; ++i) {
    const RectF row = RowRect(i);
    const VirtualListItemKind kind = KindAt(i);
    VirtualListItemState st;
    st.index = i;
    st.selected = (i == selected_index_);
    st.hovered = (i == hover_index_);
    st.focused_row = (i == selected_index_);
    st.checked = CheckedAt(i);

    if (kind == VirtualListItemKind::Custom) {
      if (paint_fn_) {
        if (st.selected || st.hovered) {
          canvas.FillRect(row, st.selected ? th.accent : th.accent_soft);
        }
        paint_fn_(canvas, row, st);
      } else {
        PaintDefaultRow(canvas, row, kind, st);
      }
    } else {
      PaintDefaultRow(canvas, row, kind, st);
      if (paint_fn_) {
        paint_fn_(canvas, row, st);
      }
    }
  }

  canvas.PopAxisAlignedClip();

  SyncVScrollBar();
  SyncHScrollBar();
  vscroll_.Paint(canvas);
  hscroll_.Paint(canvas);
}

void VirtualList::OnMouseWheel(const MouseEvent& e) {
  if (e.wheel_delta == 0) {
    return;
  }
  const float lines =
      static_cast<float>(e.wheel_delta) / static_cast<float>(WHEEL_DELTA);
  if (e.shift && MaxScrollX() > 0.f) {
    scroll_x_ = std::clamp(scroll_x_ - lines * 48.f, 0.f, MaxScrollX());
    return;
  }
  if (!NeedsScrollbar()) {
    return;
  }
  const float step = std::max(28.f, row_height(VirtualListItemKind::Text));
  set_scroll_offset(scroll_y_ - lines * step * 3.f);
}

void VirtualList::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  if (HandleHeaderMouseDown(e)) {
    return;
  }
  SyncVScrollBar();
  SyncHScrollBar();
  if (hscroll_.OnMouseDown(e)) {
    scroll_x_ = hscroll_.scroll_offset();
    return;
  }
  if (vscroll_.OnMouseDown(e)) {
    set_scroll_offset(vscroll_.scroll_offset());
    return;
  }

  const int idx = IndexAtPoint(e.x, e.y);
  if (idx < 0) {
    return;
  }
  hover_index_ = idx;
  if (HitCheckBox(idx, e.x, e.y)) {
    SetCheckedAt(idx, !CheckedAt(idx));
    set_selected_index(idx, false);
    return;
  }
  set_selected_index(idx, true);
}

void VirtualList::OnMouseMove(const MouseEvent& e) {
  if (resizing_col_ || HeaderResizeCursorAt(e.x, e.y)) {
    SetCursor(LoadCursorW(nullptr, IDC_SIZEWE));
  }
  if (HandleHeaderMouseMove(e)) {
    Invalidate();
    return;
  }
  if (hscroll_.OnMouseMove(e)) {
    scroll_x_ = hscroll_.scroll_offset();
    Invalidate();
    return;
  }
  if (vscroll_.OnMouseMove(e)) {
    set_scroll_offset(vscroll_.scroll_offset());
    Invalidate();
    return;
  }
  const int next = IndexAtPoint(e.x, e.y);
  if (next != hover_index_) {
    hover_index_ = next;
    Invalidate();
  }
}

void VirtualList::OnMouseUp(const MouseEvent& e) {
  hscroll_.OnMouseUp(e);
  vscroll_.OnMouseUp(e);
  resizing_col_ = false;
  resize_col_ = -1;
}

void VirtualList::OnMouseLeave(const MouseEvent&) {
  hover_index_ = -1;
  resizing_col_ = false;
  Invalidate();
}

void VirtualList::OnKey(const KeyEvent& e) {
  if (!e.down || Count() <= 0) {
    return;
  }
  if (e.vk == VK_RETURN) {
    CommitSelection();
    return;
  }
  if (e.vk == VK_SPACE && selected_index_ >= 0 &&
      KindAt(selected_index_) == VirtualListItemKind::Checkable) {
    SetCheckedAt(selected_index_, !CheckedAt(selected_index_));
    return;
  }
  if (e.vk == VK_UP) {
    if (selected_index_ < 0) {
      set_selected_index(0, false);
    } else if (selected_index_ > 0) {
      set_selected_index(selected_index_ - 1, false);
    }
  } else if (e.vk == VK_DOWN) {
    if (selected_index_ < 0) {
      set_selected_index(0, false);
    } else if (selected_index_ + 1 < Count()) {
      set_selected_index(selected_index_ + 1, false);
    }
  } else if (e.vk == VK_HOME) {
    set_selected_index(0, false);
  } else if (e.vk == VK_END) {
    set_selected_index(Count() - 1, false);
  } else if (e.vk == VK_PRIOR) {
    const int page = std::max(1, static_cast<int>(ViewportHeight() /
                                                  row_height(VirtualListItemKind::Text)));
    set_selected_index(std::max(0, selected_index_ - page), false);
  } else if (e.vk == VK_NEXT) {
    const int page = std::max(1, static_cast<int>(ViewportHeight() /
                                                  row_height(VirtualListItemKind::Text)));
    set_selected_index(std::min(Count() - 1, selected_index_ + page), false);
  }
}

}  // namespace auralite::ui
