#pragma once

#include "auralite/ui/node.h"

#include <functional>
#include <string>
#include <vector>

namespace auralite::ui {

// Vertical single-select string list. Place inside ScrollView for scrolling.
class ListView : public Node {
 public:
  using SelectionHandler = std::function<void(int index)>;

  ListView();

  ListView& font_size(float size);
  ListView& on_selection_changed(SelectionHandler handler);

  int AddItem(const std::wstring& text);
  void ClearItems();
  int item_count() const { return static_cast<int>(items_.size()); }

  void set_selected_index(int index);
  int selected_index() const { return selected_index_; }

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  float ItemHeight() const;
  int IndexAtY(float y) const;

  static constexpr float kItemPaddingX = 8.f;
  static constexpr float kItemPaddingY = 4.f;
  static constexpr float kMinItemHeight = 24.f;

  std::vector<std::wstring> items_;
  int selected_index_ = -1;
  float font_size_ = 14.f;
  SelectionHandler on_selection_;
  ColorF text_color_ = ColorF::FromRgb(20, 20, 20);
  ColorF selected_bg_ = ColorF::FromRgb(51, 120, 210);
  ColorF selected_text_ = ColorF::FromRgb(255, 255, 255);
};

}  // namespace auralite::ui
