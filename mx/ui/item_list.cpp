#include "mx/ui/item_list.h"

#include "mx/ui/theme.h"

#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace mx::ui {

ItemList::ItemList() {
  set_focusable(true);
  fill_width();
  fixed_height(160.f);
}

AccRole ItemList::acc_role() const {
  if (acc_role_override_) {
    return *acc_role_override_;
  }
  return AccRole::List;
}

int ItemList::AddItem(PaintItemFn paint) {
  items_.push_back(Item{std::move(paint)});
  ClampScroll();
  return static_cast<int>(items_.size()) - 1;
}

void ItemList::set_item_count(int n) {
  n = std::max(0, n);
  const int cur = item_count();
  if (n == cur) {
    return;
  }
  if (n < cur) {
    items_.resize(static_cast<size_t>(n));
    if (selected_index_ >= n) {
      selected_index_ = n > 0 ? n - 1 : -1;
    }
    if (hover_index_ >= n) {
      hover_index_ = -1;
    }
  } else {
    items_.resize(static_cast<size_t>(n));
  }
  ClampScroll();
  InvalidateSlotBinds();
}

bool ItemList::RemoveItem(int index) {
  if (index < 0 || index >= item_count()) {
    return false;
  }
  items_.erase(items_.begin() + index);
  if (selected_index_ == index) {
    selected_index_ = -1;
  } else if (selected_index_ > index) {
    --selected_index_;
  }
  if (hover_index_ == index) {
    hover_index_ = -1;
  } else if (hover_index_ > index) {
    --hover_index_;
  }
  ClampScroll();
  InvalidateSlotBinds();
  return true;
}

void ItemList::ClearItems() {
  items_.clear();
  selected_index_ = -1;
  hover_index_ = -1;
  scroll_y_ = 0.f;
  ForwardHover(MouseEvent{}, nullptr);
  for (auto& slot : slots_) {
    if (slot.root) {
      slot.root->set_event_parent(nullptr);
    }
  }
  slots_.clear();
  forward_capture_ = nullptr;
}

ItemList& ItemList::on_paint_item(PaintItemFn fn) {
  default_paint_ = std::move(fn);
  return *this;
}

ItemList& ItemList::on_bind_item(BindItemFn fn) {
  on_bind_ = std::move(fn);
  InvalidateSlotBinds();
  return *this;
}

ItemList& ItemList::item_template_factory(ItemTemplateFactory fn) {
  template_factory_ = std::move(fn);
  ForwardHover(MouseEvent{}, nullptr);
  for (auto& slot : slots_) {
    if (slot.root) {
      slot.root->set_event_parent(nullptr);
    }
  }
  slots_.clear();
  forward_capture_ = nullptr;
  return *this;
}

void ItemList::InvalidateBinds() {
  InvalidateSlotBinds();
  SyncVisibleRows();
}

ItemList& ItemList::on_selection_changed(SelectionHandler handler) {
  on_selection_ = std::move(handler);
  return *this;
}

ItemList& ItemList::row_height(float h) {
  row_h_ = std::max(1.f, h);
  ClampScroll();
  return *this;
}

ItemList& ItemList::overscan(int rows) {
  overscan_ = std::max(0, rows);
  return *this;
}

ItemList& ItemList::row_padding(float pad) {
  row_pad_ = std::max(0.f, pad);
  return *this;
}

ItemList& ItemList::columns(std::vector<ListColumn> cols) {
  columns_ = std::move(cols);
  ClampScrollX();
  ClampScroll();
  return *this;
}

ItemList& ItemList::show_header(bool v) {
  show_header_ = v;
  ClampScroll();
  return *this;
}

ItemList& ItemList::header_height(float h) {
  header_h_ = std::max(0.f, h);
  ClampScroll();
  return *this;
}

ItemList& ItemList::frozen_count(int n) {
  frozen_count_ = std::max(0, n);
  ClampScrollX();
  return *this;
}

ItemList& ItemList::on_sort_changed(SortHandler handler) {
  on_sort_ = std::move(handler);
  return *this;
}

void ItemList::set_sort(int col, ListSortDir dir, bool notify) {
  if (col < -1 || col >= static_cast<int>(columns_.size())) {
    return;
  }
  sort_col_ = col;
  sort_dir_ = (col < 0) ? ListSortDir::None : dir;
  if (notify && on_sort_) {
    on_sort_(sort_col_, sort_dir_);
  }
}

std::vector<RectF> ItemList::ColumnRects() const {
  return HeaderColumnCells(
      RectF{bounds_.x, BodyTop(), ViewportWidth(), row_h_}, columns_,
      frozen_count_, scroll_x_);
}

float ItemList::HeaderBand() const {
  return (show_header_ && !columns_.empty()) ? header_h_ : 0.f;
}

float ItemList::BodyTop() const { return bounds_.y + HeaderBand(); }

RectF ItemList::HeaderBandRect() const {
  return RectF{bounds_.x, bounds_.y, ViewportWidth(), HeaderBand()};
}

ListHeaderPaintState ItemList::MakeHeaderPaintState() const {
  ListHeaderPaintState st;
  st.sort_col = sort_col_;
  st.sort_dir = sort_dir_;
  st.frozen_count = frozen_count_;
  st.scroll_x = scroll_x_;
  return st;
}

float ItemList::MaxScrollX() const {
  const float cw = ColumnsContentWidth(ViewportWidth(), columns_);
  return std::max(0.f, cw - ViewportWidth());
}

void ItemList::ClampScrollX() {
  scroll_x_ = std::clamp(scroll_x_, 0.f, MaxScrollX());
}

void ItemList::CycleSort(int col) {
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
}

bool ItemList::HandleHeaderMouseDown(const MouseEvent& e) {
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
  return true;  // consume header-band clicks
}

bool ItemList::HandleHeaderMouseMove(const MouseEvent& e) {
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

void ItemList::CommitSelection() {
  if (on_selection_ && selected_index_ >= 0) {
    on_selection_(selected_index_);
  }
}

void ItemList::set_selected_index(int index, bool notify) {
  if (index < -1 || index >= item_count()) {
    return;
  }
  if (selected_index_ == index) {
    if (notify) {
      CommitSelection();
    }
    return;
  }
  selected_index_ = index;
  InvalidateSlotBinds();
  if (selected_index_ >= 0) {
    EnsureVisible(selected_index_);
  }
  SyncVisibleRows();
  if (notify) {
    CommitSelection();
  }
}

void ItemList::EnsureVisible(int index) {
  if (index < 0 || index >= item_count()) {
    return;
  }
  const float top = static_cast<float>(index) * row_h_;
  const float bottom = top + row_h_;
  const float view_h = ViewportHeight();
  if (top < scroll_y_) {
    set_scroll_offset(top);
  } else if (bottom > scroll_y_ + view_h) {
    set_scroll_offset(bottom - view_h);
  }
}

bool ItemList::HeaderResizeCursorAt(float x, float y) const {
  if (HeaderBand() <= 0.f || columns_.empty()) {
    return false;
  }
  return HitHeaderSplitter(x, y, HeaderBandRect(), columns_, frozen_count_,
                           scroll_x_) >= 0;
}

float ItemList::ContentHeight() const {
  return row_h_ * static_cast<float>(items_.size());
}

float ItemList::ViewportWidth() const {
  return NeedsScrollbar()
             ? std::max(0.f, bounds_.w - VerticalScrollbar::kWidth)
             : bounds_.w;
}

float ItemList::ViewportHeight() const {
  float h = std::max(0.f, bounds_.h - HeaderBand());
  if (NeedsHScrollbar()) {
    h = std::max(0.f, h - HorizontalScrollbar::kHeight);
  }
  return h;
}

float ItemList::MaxScrollOffset() const {
  return std::max(0.f, ContentHeight() - ViewportHeight());
}

void ItemList::ResolveScrollNeeds(bool* need_v, bool* need_h) const {
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

bool ItemList::NeedsScrollbar() const {
  bool need_v = false;
  bool need_h = false;
  ResolveScrollNeeds(&need_v, &need_h);
  return need_v;
}

bool ItemList::NeedsHScrollbar() const {
  bool need_v = false;
  bool need_h = false;
  ResolveScrollNeeds(&need_v, &need_h);
  return need_h;
}

void ItemList::ClampScroll() {
  scroll_y_ = std::clamp(scroll_y_, 0.f, MaxScrollOffset());
}

void ItemList::set_scroll_offset(float y) {
  scroll_y_ = y;
  ClampScroll();
}

void ItemList::SyncVScrollBar() {
  vscroll_.set_content_height(ContentHeight());
  vscroll_.set_viewport_height(ViewportHeight());
  vscroll_.set_scroll_offset(scroll_y_);
  vscroll_.set_track_bounds(
      RectF{bounds_.x + bounds_.w - VerticalScrollbar::kWidth, BodyTop(),
            VerticalScrollbar::kWidth, ViewportHeight()});
  scroll_y_ = vscroll_.scroll_offset();
}

void ItemList::SyncHScrollBar() {
  const float vw = ViewportWidth();
  hscroll_.set_content_width(ColumnsContentWidth(vw, columns_));
  hscroll_.set_viewport_width(vw);
  hscroll_.set_scroll_offset(scroll_x_);
  hscroll_.set_track_bounds(
      RectF{bounds_.x, bounds_.y + bounds_.h - HorizontalScrollbar::kHeight, vw,
            HorizontalScrollbar::kHeight});
  scroll_x_ = hscroll_.scroll_offset();
}

void ItemList::VisibleRange(int* first, int* last) const {
  const int n = item_count();
  if (n <= 0 || row_h_ <= 0.f) {
    *first = 0;
    *last = -1;
    return;
  }
  *first = std::max(0, static_cast<int>(scroll_y_ / row_h_) - overscan_);
  *last = std::min(
      n - 1,
      static_cast<int>((scroll_y_ + ViewportHeight()) / row_h_) + overscan_);
}

RectF ItemList::RowRect(int index) const {
  const float top = static_cast<float>(index) * row_h_ - scroll_y_;
  return RectF{bounds_.x, BodyTop() + top, ViewportWidth(), row_h_};
}

int ItemList::IndexAtPoint(float x, float y) const {
  const RectF vp{bounds_.x, BodyTop(), ViewportWidth(), ViewportHeight()};
  if (!ContainsPoint(vp, x, y)) {
    return -1;
  }
  if (row_h_ <= 0.f) {
    return -1;
  }
  const int idx = static_cast<int>(((y - BodyTop()) + scroll_y_) / row_h_);
  if (idx < 0 || idx >= item_count()) {
    return -1;
  }
  return idx;
}

bool ItemList::ItemUsesTemplate(int index) const {
  if (!template_factory_ || index < 0 || index >= item_count()) {
    return false;
  }
  return !items_[static_cast<size_t>(index)].paint;
}

ItemListRowState ItemList::MakeRowState(int index) const {
  ItemListRowState st;
  st.index = index;
  st.selected = (index == selected_index_);
  st.hovered = (index == hover_index_);
  st.focused_row = (index == selected_index_) && focused();
  return st;
}

void ItemList::InvalidateSlotBinds() {
  for (auto& slot : slots_) {
    slot.bound_index = -1;
  }
}

void ItemList::AttachSlotRoot(Node* root) {
  if (root) {
    root->set_event_parent(this);
  }
}

void ItemList::ForwardHover(const MouseEvent& e, Node* next) {
  if (forward_hover_ == next) {
    return;
  }
  if (forward_hover_) {
    forward_hover_->OnMouseLeave(e);
    forward_hover_->Invalidate();
  }
  forward_hover_ = next;
  if (forward_hover_) {
    forward_hover_->OnMouseEnter(e);
    forward_hover_->Invalidate();
  }
}

Node* ItemList::HitTestSlotChild(float x, float y) {
  SyncVisibleRows();
  int first = 0;
  int last = -1;
  VisibleRange(&first, &last);
  for (int i = last; i >= first; --i) {
    if (!ItemUsesTemplate(i)) {
      continue;
    }
    const int slot_i = i - first;
    if (slot_i < 0 || slot_i >= static_cast<int>(slots_.size())) {
      continue;
    }
    RowSlot& slot = slots_[static_cast<size_t>(slot_i)];
    if (!slot.root || slot.bound_index != i) {
      continue;
    }
    if (Node* hit = slot.root->HitTest(x, y)) {
      return hit;
    }
  }
  return nullptr;
}

void ItemList::SyncVisibleRows() {
  if (!template_factory_) {
    ForwardHover(MouseEvent{}, nullptr);
    for (auto& slot : slots_) {
      if (slot.root) {
        slot.root->set_event_parent(nullptr);
      }
    }
    slots_.clear();
    return;
  }

  int first = 0;
  int last = -1;
  VisibleRange(&first, &last);
  if (last < first) {
    for (auto& slot : slots_) {
      slot.bound_index = -1;
    }
    return;
  }

  const int need = last - first + 1;
  while (static_cast<int>(slots_.size()) < need) {
    RowSlot slot;
    slot.root = template_factory_();
    AttachSlotRoot(slot.root.get());
    slots_.push_back(std::move(slot));
  }

  for (int i = 0; i < need; ++i) {
    const int index = first + i;
    RowSlot& slot = slots_[static_cast<size_t>(i)];
    if (!slot.root) {
      slot.root = template_factory_();
      AttachSlotRoot(slot.root.get());
      slot.bound_index = -1;
    } else {
      AttachSlotRoot(slot.root.get());
    }
    if (!ItemUsesTemplate(index)) {
      slot.bound_index = -1;
      continue;
    }
    if (slot.bound_index != index) {
      slot.bound_index = index;
      if (on_bind_) {
        on_bind_(index, *slot.root, MakeRowState(index));
      }
    }
    const RectF row = RowRect(index);
    const float pad = row_pad_;
    const RectF inner{row.x + pad, row.y + 2.f, std::max(0.f, row.w - pad * 2.f),
                      std::max(0.f, row.h - 4.f)};
    slot.root->Measure(inner.w, inner.h);
    slot.root->Layout(inner);
  }
  for (size_t i = static_cast<size_t>(need); i < slots_.size(); ++i) {
    slots_[i].bound_index = -1;
  }
}

SizeF ItemList::Measure(float max_w, float max_h) {
  const float hug_h =
      preferred_height() > 0.f ? preferred_height() : 160.f;
  return ResolveSize(max_w, max_h,
                     preferred_width() > 0.f ? preferred_width() : max_w, hug_h);
}

void ItemList::Layout(const RectF& final_rect) {
  bounds_ = final_rect;
  ClampScroll();
  SyncVisibleRows();
}

void ItemList::PaintRow(mx::Canvas& canvas, int index, const RectF& row,
                        const ItemListRowState& state) {
  const PaintItemFn& paint =
      items_[static_cast<size_t>(index)].paint
          ? items_[static_cast<size_t>(index)].paint
          : default_paint_;
  if (paint) {
    paint(canvas, row, state);
    return;
  }
  const ThemeTokens& th = Theme::Active();
  if (state.selected) {
    canvas.FillRect(row, th.accent);
  } else if (state.hovered) {
    canvas.FillRect(row, th.accent_soft);
  }
  const ColorF tc = state.selected ? th.text_on_accent : th.text;
  canvas.DrawText(L"item " + std::to_wstring(index),
                  RectF{row.x + 8.f, row.y, std::max(0.f, row.w - 16.f), row.h},
                  tc, th.font_size, th.font_ui.c_str(),
                  mx::TextHAlign::Left);
}

void ItemList::Paint(mx::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  SyncVisibleRows();

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
    const ItemListRowState st = MakeRowState(i);
    const RectF row = RowRect(i);

    if (ItemUsesTemplate(i)) {
      if (st.selected) {
        canvas.FillRect(row, th.accent);
      } else if (st.hovered) {
        canvas.FillRect(row, th.accent_soft);
      }
      if (st.focused_row) {
        canvas.DrawRect(row, th.border_focus, 1.5f);
      }
      const int slot_i = i - first;
      if (slot_i >= 0 && slot_i < static_cast<int>(slots_.size())) {
        RowSlot& slot = slots_[static_cast<size_t>(slot_i)];
        if (slot.root && slot.bound_index == i) {
          slot.root->Paint(canvas);
        }
      }
      canvas.DrawLine(row.x, row.y + row.h - 0.5f, row.x + row.w,
                      row.y + row.h - 0.5f, th.divider, 1.f);
    } else {
      PaintRow(canvas, i, row, st);
    }
  }

  canvas.PopAxisAlignedClip();

  SyncVScrollBar();
  SyncHScrollBar();
  vscroll_.Paint(canvas);
  hscroll_.Paint(canvas);
}

Node* ItemList::HitTest(float x, float y) {
  // Always claim the list so focus/wheel/hover stay on ItemList; child events
  // are forwarded in OnMouse*.
  if (!visible() || !ContainsPoint(bounds_, x, y)) {
    return nullptr;
  }
  return this;
}

void ItemList::OnDeviceLost() {
  Node::OnDeviceLost();
  for (auto& slot : slots_) {
    if (slot.root) {
      slot.root->OnDeviceLost();
    }
  }
}

void ItemList::OnMouseWheel(const MouseEvent& e) {
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
  set_scroll_offset(scroll_y_ - lines * row_h_ * 3.f);
  SyncVisibleRows();
}

void ItemList::OnMouseDown(const MouseEvent& e) {
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
    forward_capture_ = nullptr;
    return;
  }
  if (vscroll_.OnMouseDown(e)) {
    set_scroll_offset(vscroll_.scroll_offset());
    SyncVisibleRows();
    forward_capture_ = nullptr;
    return;
  }

  const int idx = IndexAtPoint(e.x, e.y);
  if (idx < 0) {
    return;
  }
  hover_index_ = idx;
  set_selected_index(idx, true);

  Node* child = HitTestSlotChild(e.x, e.y);
  forward_capture_ = child;
  if (child) {
    child->OnMouseDown(e);
  }
}

void ItemList::OnMouseMove(const MouseEvent& e) {
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
    SyncVisibleRows();
    Invalidate();
    return;
  }

  const int next = IndexAtPoint(e.x, e.y);
  if (next != hover_index_) {
    hover_index_ = next;
    Invalidate();
  }

  if (forward_capture_) {
    forward_capture_->OnMouseMove(e);
    return;
  }

  Node* child = HitTestSlotChild(e.x, e.y);
  ForwardHover(e, child);
  if (child) {
    child->OnMouseMove(e);
  }
}

void ItemList::OnMouseUp(const MouseEvent& e) {
  resizing_col_ = false;
  resize_col_ = -1;
  hscroll_.OnMouseUp(e);
  vscroll_.OnMouseUp(e);
  if (forward_capture_) {
    forward_capture_->OnMouseUp(e);
    forward_capture_ = nullptr;
  }
}

void ItemList::OnMouseLeave(const MouseEvent& e) {
  hover_index_ = -1;
  resizing_col_ = false;
  ForwardHover(e, nullptr);
  Invalidate();
  forward_capture_ = nullptr;
}

void ItemList::OnKey(const KeyEvent& e) {
  if (!e.down || items_.empty()) {
    return;
  }
  if (e.vk == VK_RETURN || e.vk == VK_SPACE) {
    CommitSelection();
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
    } else if (selected_index_ + 1 < item_count()) {
      set_selected_index(selected_index_ + 1, false);
    }
  } else if (e.vk == VK_HOME) {
    set_selected_index(0, false);
  } else if (e.vk == VK_END) {
    set_selected_index(item_count() - 1, false);
  } else if (e.vk == VK_PRIOR) {
    const int page = std::max(1, static_cast<int>(ViewportHeight() / row_h_));
    set_selected_index(std::max(0, selected_index_ - page), false);
  } else if (e.vk == VK_NEXT) {
    const int page = std::max(1, static_cast<int>(ViewportHeight() / row_h_));
    set_selected_index(std::min(item_count() - 1, selected_index_ + page),
                       false);
  }
}

}  // namespace mx::ui
