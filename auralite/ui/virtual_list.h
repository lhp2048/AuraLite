#pragma once

#include "auralite/ui/horizontal_scrollbar.h"
#include "auralite/ui/list_columns.h"
#include "auralite/ui/node.h"
#include "auralite/ui/vertical_scrollbar.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace auralite::ui {

enum class VirtualListItemKind {
  Text,
  IconText,
  TwoLine,
  Checkable,
  Custom,
};

struct VirtualListItemState {
  int index = -1;
  bool selected = false;
  bool hovered = false;
  bool focused_row = false;
  bool checked = false;
};

class VirtualList : public Node {
 public:
  using ItemCountFn = std::function<int()>;
  using ItemKindFn = std::function<VirtualListItemKind(int index)>;
  using ItemTextFn = std::function<std::wstring(int index)>;
  using ItemSubTextFn = std::function<std::wstring(int index)>;
  using ItemCellTextFn = std::function<std::wstring(int index, int col)>;
  using ItemCheckedFn = std::function<bool(int index)>;
  using ItemCheckSetFn = std::function<void(int index, bool checked)>;
  using PaintItemFn = std::function<void(auralite::Canvas& canvas,
                                         const RectF& row,
                                         const VirtualListItemState& state)>;
  using SelectionHandler = std::function<void(int index)>;
  using CheckHandler = std::function<void(int index, bool checked)>;

  VirtualList();

  AccRole acc_role() const override;

  VirtualList& item_count(ItemCountFn fn);
  VirtualList& item_kind(ItemKindFn fn);
  VirtualList& item_text(ItemTextFn fn);
  VirtualList& item_sub_text(ItemSubTextFn fn);
  VirtualList& item_cell_text(ItemCellTextFn fn);
  VirtualList& item_checked(ItemCheckedFn fn);
  VirtualList& item_set_checked(ItemCheckSetFn fn);
  VirtualList& on_paint_item(PaintItemFn fn);
  VirtualList& on_selection_changed(SelectionHandler handler);
  VirtualList& on_check_changed(CheckHandler handler);

  VirtualList& columns(std::vector<ListColumn> cols);
  const std::vector<ListColumn>& columns() const { return columns_; }
  VirtualList& show_header(bool v);
  bool show_header() const { return show_header_; }
  VirtualList& header_height(float h);
  float header_height() const { return header_h_; }
  VirtualList& frozen_count(int n);
  int frozen_count() const { return frozen_count_; }
  using SortHandler = std::function<void(int col, ListSortDir dir)>;
  VirtualList& on_sort_changed(SortHandler handler);
  void set_sort(int col, ListSortDir dir, bool notify = true);
  int sort_column() const { return sort_col_; }
  ListSortDir sort_dir() const { return sort_dir_; }
  std::vector<RectF> ColumnRects() const;

  VirtualList& row_height(VirtualListItemKind kind, float h);
  float row_height(VirtualListItemKind kind) const;
  VirtualList& font_size(float size);
  VirtualList& overscan(int rows);

  void InvalidateData();
  void set_selected_index(int index, bool notify = true);
  int selected_index() const { return selected_index_; }
  void EnsureVisible(int index);

  bool HeaderResizeCursorAt(float x, float y) const;

  // Multi-column hit testing (DataGrid).
  bool HitCellAtPoint(float x, float y, int* row, int* col) const;
  RectF CellRectAt(int row, int col) const;

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;
  void Paint(auralite::Canvas& canvas) override;

  bool WantsMouseWheel() const override { return true; }
  void OnMouseWheel(const MouseEvent& e) override;
  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  static constexpr float kPadX = 8.f;
  static constexpr int kKindCount = 5;

  int Count() const;
  VirtualListItemKind KindAt(int index) const;
  std::wstring TextAt(int index) const;
  std::wstring SubTextAt(int index) const;
  std::wstring CellTextAt(int index, int col) const;
  bool CheckedAt(int index) const;
  void SetCheckedAt(int index, bool checked);

  void RebuildPrefixIfNeeded();
  float HeaderBand() const;
  float BodyTop() const;
  float ContentHeight() const;
  float RowTop(int index) const;
  float RowHeightAt(int index) const;
  float ViewportWidth() const;
  float ViewportHeight() const;
  float MaxScrollOffset() const;
  bool NeedsScrollbar() const;
  bool NeedsHScrollbar() const;
  void ClampScroll();
  void set_scroll_offset(float y);
  void SyncVScrollBar();
  void SyncHScrollBar();
  void ResolveScrollNeeds(bool* need_v, bool* need_h) const;
  void VisibleRange(int* first, int* last) const;
  int IndexAtContentY(float content_y) const;
  int IndexAtPoint(float x, float y) const;
  bool HitCheckBox(int index, float x, float y) const;
  RectF RowRect(int index) const;
  RectF CheckBoxRect(const RectF& row) const;

  void PaintDefaultRow(auralite::Canvas& canvas, const RectF& row,
                       VirtualListItemKind kind,
                       const VirtualListItemState& state);
  void CommitSelection();
  RectF HeaderBandRect() const;
  ListHeaderPaintState MakeHeaderPaintState() const;
  void ClampScrollX();
  float MaxScrollX() const;
  bool HandleHeaderMouseDown(const MouseEvent& e);
  bool HandleHeaderMouseMove(const MouseEvent& e);
  void CycleSort(int col);

  ItemCountFn count_fn_;
  ItemKindFn kind_fn_;
  ItemTextFn text_fn_;
  ItemSubTextFn sub_text_fn_;
  ItemCellTextFn cell_text_fn_;
  ItemCheckedFn checked_fn_;
  ItemCheckSetFn set_checked_fn_;
  PaintItemFn paint_fn_;
  SelectionHandler on_selection_;
  CheckHandler on_check_;
  SortHandler on_sort_;
  std::vector<ListColumn> columns_;
  bool show_header_ = false;
  float header_h_ = 28.f;
  int frozen_count_ = 0;
  float scroll_x_ = 0.f;
  int sort_col_ = -1;
  ListSortDir sort_dir_ = ListSortDir::None;
  bool resizing_col_ = false;
  int resize_col_ = -1;
  float resize_anchor_x_ = 0.f;
  float resize_anchor_w_ = 0.f;
  float min_col_w_ = 40.f;

  float heights_[kKindCount] = {28.f, 36.f, 48.f, 28.f, 40.f};
  std::optional<float> font_size_;
  int overscan_ = 2;
  float scroll_y_ = 0.f;
  int selected_index_ = -1;
  int hover_index_ = -1;
  bool prefix_dirty_ = true;
  std::vector<float> prefix_y_;
  float content_h_ = 0.f;
  VerticalScrollbar vscroll_;
  HorizontalScrollbar hscroll_;
};

}  // namespace auralite::ui
