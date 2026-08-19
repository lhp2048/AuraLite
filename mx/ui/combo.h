#pragma once

#include "mx/ui/node.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace mx::ui {

class Window;

// Dropdown; requires BindWindow. Single or multi (checkable popup).
class Combo : public Node {
 public:
  using ChangeHandler = std::function<void(int index)>;
  using MultiChangeHandler =
      std::function<void(const std::vector<int>& indices)>;

  Combo();

  void BindWindow(Window* window);

  Combo& items(std::vector<std::wstring> values);
  Combo& add_item(std::wstring text);
  Combo& selected(int index);
  int selected() const { return selected_; }
  Combo& selected_indices(std::vector<int> indices);
  const std::vector<int>& selected_indices() const { return selected_indices_; }
  Combo& on_changed(ChangeHandler handler);
  Combo& on_multi_changed(MultiChangeHandler handler);
  Combo& font_size(float size);
  Combo& editable(bool enable);
  bool editable() const { return editable_; }
  Combo& multi(bool enable);
  bool multi() const { return multi_; }

  const std::vector<std::wstring>& items() const { return items_; }
  bool is_open() const { return open_; }

  AccRole acc_role() const override;
  std::wstring AccValue() const override;
  bool AccSetValue(const std::wstring& value) override;
  bool AccIsExpanded() const override;
  bool AccExpand() override;
  bool AccCollapse() override;
  std::wstring AccDefaultName() const override;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(mx::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;
  void OnChar(wchar_t ch) override;
  bool ConsumesEnter() const override { return true; }

  void ClosePopup();

 private:
  void OpenPopup();
  void SelectIndex(int index, bool notify);
  void SetMultiFromPopup();
  void NotifyMulti();
  void NavigatePopup(int delta);
  void CommitPopupSelection();
  bool ItemMatchesFilter(const std::wstring& text) const;
  bool IsIndexSelected(int index) const;
  std::wstring SummaryLabel() const;
  class ListView* PopupList() const;

  Window* window_ = nullptr;
  std::vector<std::wstring> items_;
  std::vector<int> popup_index_map_;
  std::vector<int> selected_indices_;
  int selected_ = -1;
  bool open_ = false;
  bool editable_ = false;
  bool multi_ = false;
  std::wstring filter_;
  std::optional<float> font_size_;
  ChangeHandler on_changed_;
  MultiChangeHandler on_multi_changed_;
};

}  // namespace mx::ui
