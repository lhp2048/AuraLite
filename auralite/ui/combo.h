#pragma once

#include "auralite/ui/node.h"

#include <functional>
#include <string>
#include <vector>

namespace auralite::ui {

class Window;

// Single-select dropdown; requires BindWindow for popup list.
class Combo : public Node {
 public:
  using ChangeHandler = std::function<void(int index)>;

  Combo();

  void BindWindow(Window* window);

  Combo& items(std::vector<std::wstring> values);
  Combo& add_item(std::wstring text);
  Combo& selected(int index);
  int selected() const { return selected_; }
  Combo& on_changed(ChangeHandler handler);
  Combo& font_size(float size);

  const std::vector<std::wstring>& items() const { return items_; }
  bool is_open() const { return open_; }

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

  void ClosePopup();

 private:
  void OpenPopup();
  void SelectIndex(int index, bool notify);

  Window* window_ = nullptr;
  std::vector<std::wstring> items_;
  int selected_ = -1;
  bool open_ = false;
  float font_size_ = 14.f;
  ChangeHandler on_changed_;
};

}  // namespace auralite::ui
