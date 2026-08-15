#pragma once

#include "auralite/ui/node.h"

#include <functional>
#include <string>
#include <vector>

namespace auralite::ui {

class Button : public Node {
 public:
  using ClickHandler = std::function<void()>;

  Button& text(const std::wstring& t);
  Button& font_size(float size);
  Button& on_click(ClickHandler handler);
  Button& preferred_size(float w, float h);
  // Optional icon: premul BGRA; uploaded to GPU on first paint.
  Button& icon_bgra(UINT width, UINT height, const uint8_t* bgra, UINT stride);

  const std::wstring& text() const { return text_; }

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnMouseEnter(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;

 private:
  void EnsureIcon(auralite::Canvas& canvas);
  ColorF BgColor() const;

  std::wstring text_;
  float font_size_ = 15.f;
  ClickHandler on_click_;
  float pref_w_ = 140.f;
  float pref_h_ = 40.f;

  bool hovered_ = false;
  bool pressed_ = false;

  auralite::Image icon_;
  UINT icon_w_ = 0;
  UINT icon_h_ = 0;
  std::vector<uint8_t> icon_pixels_;
  UINT icon_stride_ = 0;
};

}  // namespace auralite::ui
