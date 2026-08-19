#pragma once

#include "mx/ui/node.h"

#include <functional>
#include <optional>
#include <string>

namespace mx::ui {

// One row in a PopupHost / MenuBar menu. Command, check, radio, or separator.
class MenuItem : public Node {
 public:
  using ClickHandler = std::function<void()>;
  using ChangedHandler = std::function<void(bool checked)>;

  MenuItem();

  MenuItem& text(const std::wstring& t);
  const std::wstring& text() const { return text_; }
  MenuItem& icon(std::wstring name);
  const std::wstring& icon() const { return icon_; }
  MenuItem& separator(bool v);
  bool separator() const { return separator_; }
  MenuItem& checkable(bool v);
  bool checkable() const { return checkable_; }
  MenuItem& checked(bool v);
  bool checked() const { return checked_; }
  MenuItem& radio_group(int id);
  int radio_group() const { return radio_group_; }
  MenuItem& font_size(float size);
  MenuItem& text_color(const ColorF& c);
  MenuItem& bg_hover(const ColorF& c);
  MenuItem& on_click(ClickHandler handler);
  MenuItem& on_changed(ChangedHandler handler);

  AccRole acc_role() const override;
  AccState acc_state() const override;
  bool AccInvoke() override;
  bool AccToggle() override;
  std::wstring AccDefaultName() const override;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(mx::Canvas& canvas) override;

  void OnMouseEnter(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;
  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  void Activate();
  void SetCheckedInternal(bool v, bool notify);
  void UncheckRadioPeers();
  static void UncheckMenuItemsInTree(Node* node, MenuItem* except, int group);
  RectF GutterRect() const;

  static constexpr float kItemH = 28.f;
  static constexpr float kSepH = 9.f;
  static constexpr float kGutter = 22.f;
  static constexpr float kIcon = 16.f;
  static constexpr float kPadX = 8.f;

  std::wstring text_;
  std::wstring icon_;
  std::optional<float> font_size_;
  std::optional<ColorF> text_color_;
  std::optional<ColorF> hover_bg_;
  bool separator_ = false;
  bool checkable_ = false;
  bool checked_ = false;
  int radio_group_ = 0;
  ClickHandler on_click_;
  ChangedHandler on_changed_;
  bool hovered_ = false;
  bool pressed_ = false;
};

}  // namespace mx::ui
