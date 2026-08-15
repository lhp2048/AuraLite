#pragma once

#include "auralite/ui/node.h"

#include <memory>
#include <string>

namespace auralite::ui {

// Menu-row trigger that Push()es owned |content_| into PopupHost; host returns
// the tree via content() on DismissFrom of that layer.
class Submenu : public Node {
 public:
  Submenu();

  Submenu& text(const std::wstring& t);
  Submenu& content(std::unique_ptr<Node> node);
  Submenu& open_on_hover(bool v);

  const std::wstring& text() const { return text_; }
  Node* content() const { return content_.get(); }
  bool open_on_hover() const { return open_on_hover_; }

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseEnter(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;

 private:
  void OpenIfNeeded();
  void RequestRepaint();
  RectF AnchorScreenRect() const;

  static constexpr float kRowH = 32.f;
  static constexpr float kPadX = 12.f;
  static constexpr float kChevronGap = 8.f;
  static constexpr float kChevronSlot = 14.f;

  std::wstring text_;
  std::unique_ptr<Node> content_;
  bool open_on_hover_ = true;
  bool hovered_ = false;
};

}  // namespace auralite::ui
