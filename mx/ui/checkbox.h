#pragma once

#include "mx/ui/node.h"

#include <functional>
#include <optional>
#include <string>

namespace mx::ui {

class Checkbox : public Node {
 public:
  using ChangedHandler = std::function<void(bool checked)>;

  Checkbox();

  Checkbox& text(const std::wstring& t);
  Checkbox& font_size(float size);
  Checkbox& checked(bool v);
  Checkbox& on_changed(ChangedHandler handler);

  const std::wstring& text() const { return text_; }
  bool checked() const { return checked_; }

  AccRole acc_role() const override;
  AccState acc_state() const override;
  bool AccToggle() override;
  std::wstring AccDefaultName() const override;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(mx::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnMouseEnter(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  void Toggle();
  RectF BoxRect() const;

  static constexpr float kBoxSize = 16.f;
  static constexpr float kLabelGap = 8.f;

  std::wstring text_;
  std::optional<float> font_size_;
  bool checked_ = false;
  ChangedHandler on_changed_;
  bool pressed_ = false;
  bool hovered_ = false;
};

}  // namespace mx::ui
