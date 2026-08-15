#pragma once

#include "auralite/ui/node.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace auralite::ui {

// Vertical list. Single-select by default; checkable for Combo multi.
class ListView : public Node {
 public:
  using SelectionHandler = std::function<void(int index)>;
  using CheckHandler = std::function<void(int index, bool checked)>;

  ListView();

  ListView& font_size(float size);
  ListView& on_selection_changed(SelectionHandler handler);
  ListView& on_check_changed(CheckHandler handler);
  ListView& checkable(bool enable);
  bool checkable() const { return checkable_; }

  // Sparse color overrides; unset falls back to Theme tokens.
  ListView& text_color(const ColorF& c);
  ListView& selected_bg(const ColorF& c);
  ListView& selected_text(const ColorF& c);
  ListView& hover_bg(const ColorF& c);

  int AddItem(const std::wstring& text);
  void ClearItems();
  int item_count() const { return static_cast<int>(items_.size()); }

  // |notify| false = highlight only (arrow keys / Combo open navigation).
  void set_selected_index(int index, bool notify = true);
  int selected_index() const { return selected_index_; }
  void set_hover_index(int index);
  int hover_index() const { return hover_index_; }

  void set_checked(int index, bool checked);
  bool is_checked(int index) const;
  void set_checked_indices(const std::vector<int>& indices);
  std::vector<int> checked_indices() const;
  void ToggleChecked(int index);

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  float ItemHeight() const;
  float CheckBoxSize() const;
  int IndexAtY(float y) const;
  void CommitSelection();
  void EnsureCheckedSize();

  static constexpr float kItemPaddingX = 8.f;
  static constexpr float kItemPaddingY = 4.f;
  static constexpr float kMinItemHeight = 24.f;

  std::vector<std::wstring> items_;
  std::vector<bool> checked_;
  int selected_index_ = -1;
  int hover_index_ = -1;
  std::optional<float> font_size_;
  bool checkable_ = false;
  SelectionHandler on_selection_;
  CheckHandler on_check_;
  std::optional<ColorF> text_color_;
  std::optional<ColorF> selected_bg_;
  std::optional<ColorF> selected_text_;
  std::optional<ColorF> hover_bg_;
};

}  // namespace auralite::ui
