#pragma once

#include "mx/ui/node.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace mx::ui {

enum class ButtonVariant { Primary, Secondary, Danger };

class Button : public Node {
 public:
  using ClickHandler = std::function<void()>;

  Button();

  Button& text(const std::wstring& t);
  Button& font_size(float size);
  Button& on_click(ClickHandler handler);
  bool has_on_click() const { return static_cast<bool>(on_click_); }
  Button& preferred_size(float w, float h);
  // Semantic chrome. Clears sparse color overrides so SetActive can restyle.
  Button& variant(ButtonVariant v);
  ButtonVariant variant() const { return variant_; }
  // Sparse color overrides; unset falls back to the active variant's tokens.
  Button& bg(const ColorF& c);
  Button& bg_hover(const ColorF& c);
  Button& bg_pressed(const ColorF& c);
  // Optional label color; unset → variant default (or text if custom bg set).
  Button& text_color(const ColorF& c);
  Button& text_align(mx::TextHAlign align);
  Button& corner_radius(float r);
  // Optional icon: premul BGRA; uploaded to GPU on first paint.
  Button& icon_bgra(UINT width, UINT height, const uint8_t* bgra, UINT stride);
  Button& set_enabled(bool e);
  bool enabled() const { return enabled_; }
  // Enter on this window (except TextArea / focused Button) invokes this button.
  Button& is_default(bool v);
  bool is_default() const { return is_default_; }
  // Window shortcut (Ctrl/Alt / F-key / Esc). Invokes on_click when matched.
  Button& accelerator(KeyChord chord);
  Button& accelerator(const std::string& spec);
  const KeyChord& accelerator() const { return accelerator_; }

  AccRole acc_role() const override;
  AccState acc_state() const override;
  bool AccInvoke() override;
  bool ConsumesEnter() const override { return true; }

  const std::wstring& text() const { return text_; }

  ColorF resolved_bg() const { return BgColor(); }
  ColorF resolved_label() const { return LabelColor(); }

  SizeF Measure(float max_w, float max_h) override;
  void Paint(mx::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnMouseEnter(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;
  void OnDeviceLost() override;

  std::wstring AccDefaultName() const override;

 private:
  void EnsureIcon(mx::Canvas& canvas);
  ColorF BgColor() const;
  ColorF LabelColor() const;

  std::wstring text_;
  std::optional<float> font_size_;
  ButtonVariant variant_ = ButtonVariant::Primary;
  std::optional<ColorF> bg_;
  std::optional<ColorF> bg_hover_;
  std::optional<ColorF> bg_pressed_;
  std::optional<ColorF> text_color_;
  mx::TextHAlign text_align_ = mx::TextHAlign::Center;
  float corner_radius_ = 8.f;
  ClickHandler on_click_;

  bool hovered_ = false;
  bool pressed_ = false;
  bool enabled_ = true;
  bool is_default_ = false;
  KeyChord accelerator_{};

  mx::Image icon_;
  UINT icon_w_ = 0;
  UINT icon_h_ = 0;
  std::vector<uint8_t> icon_pixels_;
  UINT icon_stride_ = 0;
};

}  // namespace mx::ui
