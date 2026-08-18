#pragma once

#include "auralite/ui/horizontal_scrollbar.h"
#include "auralite/ui/list_columns.h"
#include "auralite/ui/node.h"
#include "auralite/ui/vertical_scrollbar.h"

#include <functional>
#include <memory>
#include <vector>

namespace auralite::ui {

struct ItemListRowState {
  int index = -1;
  bool selected = false;
  bool hovered = false;
  bool focused_row = false;
};

// Fixed items + optional shared row template (widget tree) with recycle bind;
// paints only visible rows. Without template, uses owner-draw paint callbacks.
class ItemList : public Node {
 public:
  using PaintItemFn = std::function<void(auralite::Canvas& canvas,
                                         const RectF& row,
                                         const ItemListRowState& state)>;
  using BindItemFn = std::function<void(int index, Node& row_root,
                                        const ItemListRowState& state)>;
  using ItemTemplateFactory = std::function<std::unique_ptr<Node>()>;
  using SelectionHandler = std::function<void(int index)>;

  ItemList();

  AccRole acc_role() const override;

  int AddItem(PaintItemFn paint = {});
  void set_item_count(int n);
  bool RemoveItem(int index);
  void ClearItems();
  int item_count() const { return static_cast<int>(items_.size()); }

  ItemList& on_paint_item(PaintItemFn fn);
  ItemList& on_bind_item(BindItemFn fn);
  ItemList& item_template_factory(ItemTemplateFactory fn);
  bool has_item_template() const { return static_cast<bool>(template_factory_); }

  void InvalidateBinds();

  ItemList& columns(std::vector<ListColumn> cols);
  const std::vector<ListColumn>& columns() const { return columns_; }
  ItemList& show_header(bool v);
  bool show_header() const { return show_header_; }
  ItemList& header_height(float h);
  float header_height() const { return header_h_; }
  ItemList& frozen_count(int n);
  int frozen_count() const { return frozen_count_; }
  using SortHandler = std::function<void(int col, ListSortDir dir)>;
  ItemList& on_sort_changed(SortHandler handler);
  void set_sort(int col, ListSortDir dir, bool notify = true);
  int sort_column() const { return sort_col_; }
  ListSortDir sort_dir() const { return sort_dir_; }
  std::vector<RectF> ColumnRects() const;

  ItemList& on_selection_changed(SelectionHandler handler);
  ItemList& row_height(float h);
  float row_height() const { return row_h_; }
  ItemList& overscan(int rows);
  ItemList& row_padding(float pad);
  float row_padding() const { return row_pad_; }

  void set_selected_index(int index, bool notify = true);
  int selected_index() const { return selected_index_; }
  void EnsureVisible(int index);

  float scroll_offset() const { return scroll_y_; }
  void set_scroll_offset(float y);

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;
  void Paint(auralite::Canvas& canvas) override;
  Node* HitTest(float x, float y) override;
  void OnDeviceLost() override;

  bool WantsMouseWheel() const override { return true; }
  void OnMouseWheel(const MouseEvent& e) override;
  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  struct Item {
    PaintItemFn paint;
  };

  struct RowSlot {
    std::unique_ptr<Node> root;
    int bound_index = -1;
  };

  float HeaderBand() const;
  float BodyTop() const;
  float ContentHeight() const;
  float ViewportWidth() const;
  float ViewportHeight() const;
  float MaxScrollOffset() const;
  bool NeedsScrollbar() const;
  bool NeedsHScrollbar() const;
  void ClampScroll();
  void SyncVScrollBar();
  void SyncHScrollBar();
  void ResolveScrollNeeds(bool* need_v, bool* need_h) const;
  void VisibleRange(int* first, int* last) const;
  int IndexAtPoint(float x, float y) const;
  RectF RowRect(int index) const;
  void CommitSelection();
  void PaintRow(auralite::Canvas& canvas, int index, const RectF& row,
                 const ItemListRowState& state);
  bool ItemUsesTemplate(int index) const;
  ItemListRowState MakeRowState(int index) const;
  void SyncVisibleRows();
  void InvalidateSlotBinds();
  void AttachSlotRoot(Node* root);
  Node* HitTestSlotChild(float x, float y);
  void ForwardHover(const MouseEvent& e, Node* next);

  RectF HeaderBandRect() const;
  ListHeaderPaintState MakeHeaderPaintState() const;
  void ClampScrollX();
  float MaxScrollX() const;
  bool HandleHeaderMouseDown(const MouseEvent& e);
  bool HandleHeaderMouseMove(const MouseEvent& e);
  void CycleSort(int col);

  std::vector<Item> items_;
  std::vector<ListColumn> columns_;
  bool show_header_ = false;
  float header_h_ = 28.f;
  int frozen_count_ = 0;
  float scroll_x_ = 0.f;
  int sort_col_ = -1;
  ListSortDir sort_dir_ = ListSortDir::None;
  SortHandler on_sort_;
  bool resizing_col_ = false;
  int resize_col_ = -1;
  float resize_anchor_x_ = 0.f;
  float resize_anchor_w_ = 0.f;
  float min_col_w_ = 40.f;
  PaintItemFn default_paint_;
  BindItemFn on_bind_;
  ItemTemplateFactory template_factory_;
  std::vector<RowSlot> slots_;
  SelectionHandler on_selection_;
  float row_h_ = 28.f;
  float row_pad_ = 4.f;
  int overscan_ = 2;
  float scroll_y_ = 0.f;
  int selected_index_ = -1;
  int hover_index_ = -1;
  VerticalScrollbar vscroll_;
  HorizontalScrollbar hscroll_;
  Node* forward_hover_ = nullptr;
  Node* forward_capture_ = nullptr;
};

}  // namespace auralite::ui
