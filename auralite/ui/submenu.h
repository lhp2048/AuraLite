#pragma once

#include "auralite/ui/node.h"

#include <memory>
#include <optional>
#include <string>

namespace auralite::ui {

// Menu-row trigger that Push()es owned |content_| into PopupHost; leftover on
// Push failure is returned to the caller. Host returns the tree via content()
// on DismissFrom of that layer.
class Submenu : public Node {
 public:
  Submenu();

  Submenu& text(const std::wstring& t);
  Submenu& content(std::unique_ptr<Node> node);
  Submenu& open_on_hover(bool v);
  // Sparse color overrides; unset falls back to Theme surface / accent_soft.
  Submenu& bg(const ColorF& c);
  Submenu& bg_hover(const ColorF& c);
  Submenu& text_color(const ColorF& c);
  Submenu& font_size(float size);
  Submenu& corner_radius(float r);

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
  ColorF BgColor() const;
  ColorF LabelColor() const;

  static constexpr float kRowH = 32.f;
  static constexpr float kPadX = 12.f;
  static constexpr float kChevronGap = 8.f;
  static constexpr float kChevronSlot = 14.f;

  std::wstring text_;
  std::unique_ptr<Node> content_;
  bool open_on_hover_ = true;
  bool hovered_ = false;

  std::optional<ColorF> bg_;
  std::optional<ColorF> bg_hover_;
  std::optional<ColorF> text_color_;
  std::optional<float> font_size_;
  float corner_radius_ = 0.f;
};

}  // namespace auralite::ui
