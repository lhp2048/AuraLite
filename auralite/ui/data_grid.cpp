#include "auralite/ui/data_grid.h"

#include "auralite/ui/column.h"
#include "auralite/ui/text_field.h"
#include "auralite/ui/window.h"

#include <algorithm>
#include <cmath>
#include <cwctype>

namespace auralite::ui {
namespace {

std::wstring TrimCell(const std::wstring& s) {
  size_t b = 0;
  while (b < s.size() && std::iswspace(s[b])) {
    ++b;
  }
  size_t e = s.size();
  while (e > b && std::iswspace(s[e - 1])) {
    --e;
  }
  return s.substr(b, e - b);
}

bool TryParseDouble(const std::wstring& s, double* out) {
  if (s.empty()) {
    return false;
  }
  wchar_t* end = nullptr;
  const double v = wcstod(s.c_str(), &end);
  if (end == s.c_str()) {
    return false;
  }
  while (end && *end != L'\0' && std::iswspace(*end)) {
    ++end;
  }
  if (*end != L'\0') {
    return false;
  }
  if (out) {
    *out = v;
  }
  return true;
}

int CompareTextCells(const std::wstring& ta, const std::wstring& tb) {
  if (ta < tb) {
    return -1;
  }
  if (ta > tb) {
    return 1;
  }
  return 0;
}

int CompareNumberCells(const std::wstring& ta, const std::wstring& tb) {
  double da = 0.0;
  double db = 0.0;
  const bool pa = TryParseDouble(ta, &da);
  const bool pb = TryParseDouble(tb, &db);
  if (pa && pb) {
    if (da < db) {
      return -1;
    }
    if (da > db) {
      return 1;
    }
    return 0;
  }
  if (pa != pb) {
    return pa ? -1 : 1;
  }
  return CompareTextCells(ta, tb);
}

int CompareNaturalCells(const std::wstring& a, const std::wstring& b) {
  size_t ia = 0;
  size_t ib = 0;
  while (ia < a.size() || ib < b.size()) {
    if (ia >= a.size()) {
      return -1;
    }
    if (ib >= b.size()) {
      return 1;
    }
    const bool da = std::iswdigit(a[ia]) != 0;
    const bool db = std::iswdigit(b[ib]) != 0;
    if (da && db) {
      size_t ja = ia;
      while (ja < a.size() && std::iswdigit(a[ja])) {
        ++ja;
      }
      size_t jb = ib;
      while (jb < b.size() && std::iswdigit(b[jb])) {
        ++jb;
      }
      const std::wstring na = a.substr(ia, ja - ia);
      const std::wstring nb = b.substr(ib, jb - ib);
      double va = 0.0;
      double vb = 0.0;
      TryParseDouble(na, &va);
      TryParseDouble(nb, &vb);
      if (va < vb) {
        return -1;
      }
      if (va > vb) {
        return 1;
      }
      ia = ja;
      ib = jb;
      continue;
    }
    if (a[ia] < b[ib]) {
      return -1;
    }
    if (a[ia] > b[ib]) {
      return 1;
    }
    ++ia;
    ++ib;
  }
  return 0;
}

}  // namespace

int CompareDataGridCells(const std::wstring& a, const std::wstring& b,
                         ColumnSortKind kind) {
  const std::wstring ta = TrimCell(a);
  const std::wstring tb = TrimCell(b);
  switch (kind) {
    case ColumnSortKind::Text:
      return CompareTextCells(ta, tb);
    case ColumnSortKind::Number:
      return CompareNumberCells(ta, tb);
    case ColumnSortKind::Natural:
      return CompareNaturalCells(ta, tb);
    case ColumnSortKind::Auto:
    default:
      break;
  }
  double da = 0.0;
  double db = 0.0;
  if (TryParseDouble(ta, &da) && TryParseDouble(tb, &db)) {
    if (da < db) {
      return -1;
    }
    if (da > db) {
      return 1;
    }
    return 0;
  }
  return CompareNaturalCells(ta, tb);
}

DataGrid::DataGrid() {
  set_focusable(true);
  fill_width();
  fixed_height(220.f);
  show_header(true);
  header_height(30.f);
  row_height(VirtualListItemKind::Text, 32.f);
  WireDataSource();
  item_kind([](int) { return VirtualListItemKind::Text; });
}

void DataGrid::BindWindow(Window* window) { window_ = window; }

DataGrid& DataGrid::editable(bool on) {
  editable_ = on;
  return *this;
}

void DataGrid::SyncStorageSize() {
  const int cols = static_cast<int>(columns().size());
  for (auto& row : cells_) {
    row.resize(static_cast<size_t>(cols));
  }
}

void DataGrid::WireDataSource() {
  item_count([this]() { return static_cast<int>(cells_.size()); });
  item_cell_text([this](int row, int col) { return cell(row, col); });
}

DataGrid& DataGrid::set_row_count(int rows) {
  rows = std::max(0, rows);
  cells_.resize(static_cast<size_t>(rows));
  SyncStorageSize();
  InvalidateData();
  return *this;
}

DataGrid& DataGrid::set_cell(int row, int col, std::wstring value) {
  if (row < 0 || col < 0) {
    return *this;
  }
  if (row >= static_cast<int>(cells_.size())) {
    set_row_count(row + 1);
  }
  SyncStorageSize();
  if (col >= static_cast<int>(columns().size())) {
    return *this;
  }
  cells_[static_cast<size_t>(row)][static_cast<size_t>(col)] =
      std::move(value);
  Invalidate();
  return *this;
}

std::wstring DataGrid::cell(int row, int col) const {
  if (row < 0 || col < 0 || row >= static_cast<int>(cells_.size())) {
    return {};
  }
  const auto& r = cells_[static_cast<size_t>(row)];
  if (col >= static_cast<int>(r.size())) {
    return {};
  }
  return r[static_cast<size_t>(col)];
}

DataGrid& DataGrid::on_cell_changed(CellChangedHandler handler) {
  on_cell_changed_ = std::move(handler);
  return *this;
}

DataGrid& DataGrid::auto_sort(bool on) {
  auto_sort_ = on;
  return *this;
}

DataGrid& DataGrid::sort_compare(SortCompareHandler handler) {
  sort_compare_ = std::move(handler);
  return *this;
}

DataGrid& DataGrid::on_sort_changed(SortHandler handler) {
  user_on_sort_ = std::move(handler);
  return *this;
}

void DataGrid::set_sort(int col, ListSortDir dir, bool notify) {
  VirtualList::set_sort(col, dir, false);
  HandleSortChanged(sort_column(), sort_dir());
  if (notify && user_on_sort_) {
    user_on_sort_(sort_column(), sort_dir());
  }
}

int DataGrid::CompareCells(int col, const std::wstring& a,
                           const std::wstring& b) const {
  if (sort_compare_) {
    return sort_compare_(col, a, b);
  }
  return CompareDataGridCells(a, b, EffectiveSortKind(col));
}

ColumnSortKind DataGrid::EffectiveSortKind(int col) const {
  if (col < 0 || col >= static_cast<int>(columns().size())) {
    return ColumnSortKind::Auto;
  }
  const ColumnSortKind configured =
      columns()[static_cast<size_t>(col)].sort_kind;
  if (configured != ColumnSortKind::Auto) {
    return configured;
  }
  bool any = false;
  for (const auto& row : cells_) {
    if (col >= static_cast<int>(row.size())) {
      continue;
    }
    const std::wstring v = TrimCell(row[static_cast<size_t>(col)]);
    if (v.empty()) {
      continue;
    }
    any = true;
    if (!TryParseDouble(v, nullptr)) {
      return ColumnSortKind::Natural;
    }
  }
  return any ? ColumnSortKind::Number : ColumnSortKind::Natural;
}

void DataGrid::ApplySort(int col, ListSortDir dir) {
  if (col < 0 || dir == ListSortDir::None || cells_.empty()) {
    return;
  }
  if (col >= static_cast<int>(columns().size())) {
    return;
  }
  FlushEdit(true);

  std::vector<std::wstring> sel_row;
  const int sel = selected_index();
  if (sel >= 0 && sel < static_cast<int>(cells_.size())) {
    sel_row = cells_[static_cast<size_t>(sel)];
  }

  const bool asc = dir == ListSortDir::Asc;
  std::stable_sort(cells_.begin(), cells_.end(),
                   [this, col, asc](const std::vector<std::wstring>& ra,
                                    const std::vector<std::wstring>& rb) {
                     const std::wstring va =
                         col < static_cast<int>(ra.size())
                             ? ra[static_cast<size_t>(col)]
                             : L"";
                     const std::wstring vb =
                         col < static_cast<int>(rb.size())
                             ? rb[static_cast<size_t>(col)]
                             : L"";
                     int c = CompareCells(col, va, vb);
                     if (!asc) {
                       c = -c;
                     }
                     return c < 0;
                   });

  if (!sel_row.empty()) {
    for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
      if (cells_[static_cast<size_t>(i)] == sel_row) {
        set_selected_index(i, false);
        break;
      }
    }
  }
  InvalidateData();
  Invalidate();
}

void DataGrid::HandleSortChanged(int col, ListSortDir dir) {
  if (auto_sort_) {
    ApplySort(col, dir);
  }
}

DataGrid& DataGrid::columns(std::vector<ListColumn> cols) {
  VirtualList::columns(std::move(cols));
  SyncStorageSize();
  return *this;
}

bool DataGrid::ColumnEditable(int col) const {
  if (!editable_ || col < 0 || col >= static_cast<int>(columns().size())) {
    return false;
  }
  return columns()[static_cast<size_t>(col)].editable;
}

void DataGrid::FlushEdit(bool accept) {
  if (edit_row_ < 0) {
    return;
  }
  const int row = edit_row_;
  const int col = edit_col_;
  if (accept && edit_field_) {
    const std::wstring value = edit_field_->text();
    set_cell(row, col, value);
    if (on_cell_changed_) {
      on_cell_changed_(row, col, value);
    }
  }
  edit_field_ = nullptr;
  edit_row_ = edit_col_ = -1;
}

void DataGrid::BeginEdit(int row, int col) {
  if (!window_ || !ColumnEditable(col) || row < 0) {
    return;
  }
  if (columns().empty()) {
    return;
  }
  FlushEdit(true);

  edit_row_ = row;
  edit_col_ = col;
  focus_col_ = col;
  set_selected_index(row, false);
  EnsureVisible(row);

  auto field = std::make_unique<TextField>();
  field->text(cell(row, col));
  field->fill_width();
  edit_field_ = field.get();

  auto pop = std::make_unique<Column>();
  pop->AddChild(std::move(field));

  const RectF cr = CellRectAt(row, col);
  const float w = std::max(cr.w, 140.f);
  const float h = 36.f;
  const RectF pop_rect{cr.x, cr.y + cr.h + 2.f, w, h};
  pop->Layout(pop_rect);

  edit_field_->on_submit([this] { EndEdit(true); });

  window_->SetPopup(
      std::move(pop),
      [this]() {
        if (edit_row_ >= 0) {
          FlushEdit(true);
        } else {
          edit_field_ = nullptr;
        }
      },
      this, &pop_rect);
  window_->DeferFocusNode(edit_field_);
}

void DataGrid::EndEdit(bool accept) {
  FlushEdit(accept);
  if (window_ && window_->popup()) {
    window_->RequestClearPopup();
  }
  Invalidate();
}

AccRole DataGrid::acc_role() const {
  if (acc_role_override_) {
    return *acc_role_override_;
  }
  return AccRole::List;
}

std::wstring DataGrid::AccDefaultName() const { return L"DataGrid"; }

std::wstring DataGrid::AccValue() const {
  if (selected_index() >= 0 && focus_col_ >= 0) {
    return cell(selected_index(), focus_col_);
  }
  if (selected_index() >= 0) {
    return cell(selected_index(), 0);
  }
  return {};
}

void DataGrid::OnMouseDown(const MouseEvent& e) {
  if (edit_row_ >= 0 && window_ && window_->popup()) {
    if (!window_->popup()->HitTest(e.x, e.y)) {
      EndEdit(true);
    }
  }
  int row = -1;
  int col = -1;
  if (HitCellAtPoint(e.x, e.y, &row, &col)) {
    focus_col_ = col;
  }
  const int sort_col_before = sort_column();
  const ListSortDir sort_dir_before = sort_dir();
  VirtualList::OnMouseDown(e);
  if (sort_column() != sort_col_before || sort_dir() != sort_dir_before) {
    HandleSortChanged(sort_column(), sort_dir());
    if (user_on_sort_) {
      user_on_sort_(sort_column(), sort_dir());
    }
  }
}

void DataGrid::OnMouseDoubleClick(const MouseEvent& e) {
  int row = -1;
  int col = -1;
  if (HitCellAtPoint(e.x, e.y, &row, &col)) {
    BeginEdit(row, col);
    return;
  }
  VirtualList::OnMouseDoubleClick(e);
}

void DataGrid::OnKey(const KeyEvent& e) {
  if (!e.down) {
    return;
  }
  if (edit_row_ >= 0 && e.vk == VK_ESCAPE) {
    EndEdit(false);
    return;
  }
  if (e.vk == VK_F2 && selected_index() >= 0) {
    BeginEdit(selected_index(), focus_col_);
    return;
  }
  if (e.vk == VK_RETURN && selected_index() >= 0 && ColumnEditable(focus_col_)) {
    BeginEdit(selected_index(), focus_col_);
    return;
  }
  VirtualList::OnKey(e);
}

}  // namespace auralite::ui
