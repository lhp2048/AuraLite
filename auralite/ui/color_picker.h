#pragma once

#include "auralite/canvas.h"
#include "auralite/ui/node.h"

#include <functional>
#include <optional>
#include <string>

namespace auralite::ui {

class Window;

// Field + popup color chooser (Window::SetPopup), like DatePicker.
// mode: Simple = preset swatches; Full = SV panel + hue (+ optional alpha).
enum class ColorPickerMode { Simple, Full };

class ColorPicker : public Node {
 public:
  using ChangeHandler = std::function<void(const ColorF&)>;

  ColorPicker();

  void BindWindow(Window* window);

  ColorPicker& color(const ColorF& c);
  const ColorF& color() const { return color_; }
  // "#RRGGBB" or "#RRGGBBAA". Returns false if invalid.
  bool set_hex(const std::wstring& hex);
  std::wstring hex(bool with_alpha = false) const;

  ColorPicker& mode(ColorPickerMode m);
  ColorPickerMode mode() const { return mode_; }
  // Full mode: show alpha bar and include AA in field text.
  ColorPicker& alpha(bool enable);
  bool alpha() const { return alpha_; }

  ColorPicker& font_size(float size);
  ColorPicker& on_changed(ChangeHandler handler);

  bool is_open() const { return open_; }
  void ClosePopup();

  AccRole acc_role() const override;
  std::wstring AccDefaultName() const override;
  std::wstring AccValue() const override;
  bool AccSetValue(const std::wstring& value) override;
  bool AccIsExpanded() const override;
  bool AccExpand() override;
  bool AccCollapse() override;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;
  void OnMouseDown(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  friend class ColorPopup;
  void OpenPopup();
  void Commit(const ColorF& c, bool close);
  void Notify();
  std::wstring DisplayText() const;

  Window* window_ = nullptr;
  ColorF color_ = ColorF::FromRgb(40, 110, 200);
  ColorPickerMode mode_ = ColorPickerMode::Simple;
  bool alpha_ = false;
  bool open_ = false;
  std::optional<float> font_size_;
  ChangeHandler on_changed_;
};

}  // namespace auralite::ui
