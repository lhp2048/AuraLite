#pragma once

#include "auralite/ui/list_columns.h"
#include "auralite/ui/virtual_list.h"

#include <functional>
#include <string>
#include <vector>

namespace auralite::ui {

class TextField;
class Window;

// Default cell compare for DataGrid. kind selects auto / text / number column sort.
int CompareDataGridCells(const std::wstring& a, const std::wstring& b,
                         ColumnSortKind kind = ColumnSortKind::Auto);

// Tabular data grid: VirtualList multi-column + header/sort/scroll, plus
// in-cell editing (popup TextField). Requires BindWindow before edit.
class DataGrid : public VirtualList {
 public:
  using CellChangedHandler =
      std::function<void(int row, int col, const std::wstring& value)>;
  using SortCompareHandler =
      std::function<int(int col, const std::wstring& a, const std::wstring& b)>;

  DataGrid();

  void BindWindow(Window* window);

  DataGrid& editable(bool on);
  bool editable() const { return editable_; }

  DataGrid& set_row_count(int rows);
  int row_count() const { return static_cast<int>(cells_.size()); }

  DataGrid& set_cell(int row, int col, std::wstring value);
  std::wstring cell(int row, int col) const;

  DataGrid& on_cell_changed(CellChangedHandler handler);

  // When true (default), header sort reorders built-in cells_. Custom compare via
  // sort_compare; set auto_sort(false) to handle on_sort_changed yourself.
  DataGrid& auto_sort(bool on);
  bool auto_sort() const { return auto_sort_; }

  DataGrid& sort_compare(SortCompareHandler handler);

  DataGrid& on_sort_changed(SortHandler handler);

  void set_sort(int col, ListSortDir dir, bool notify = true);

  using VirtualList::columns;
  DataGrid& columns(std::vector<ListColumn> cols);

  AccRole acc_role() const override;
  std::wstring AccDefaultName() const override;
  std::wstring AccValue() const override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseDoubleClick(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  void SyncStorageSize();
  void WireDataSource();
  bool ColumnEditable(int col) const;
  void FlushEdit(bool accept);
  void BeginEdit(int row, int col);
  void EndEdit(bool accept);
  void HandleSortChanged(int col, ListSortDir dir);
  void ApplySort(int col, ListSortDir dir);
  int CompareCells(int col, const std::wstring& a, const std::wstring& b) const;
  ColumnSortKind EffectiveSortKind(int col) const;

  Window* window_ = nullptr;
  bool editable_ = true;
  bool auto_sort_ = true;
  std::vector<std::vector<std::wstring>> cells_;
  CellChangedHandler on_cell_changed_;
  SortCompareHandler sort_compare_;
  SortHandler user_on_sort_;
  int focus_col_ = 0;
  int edit_row_ = -1;
  int edit_col_ = -1;
  TextField* edit_field_ = nullptr;
};

}  // namespace auralite::ui
