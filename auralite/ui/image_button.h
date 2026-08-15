#pragma once

#include "auralite/ui/node.h"

#include <functional>
#include <vector>

namespace auralite::ui {

// Image-only clickable control (hover / pressed chrome + on_click).
class ImageButton : public Node {
 public:
  using ClickHandler = std::function<void()>;

  ImageButton& SetPixels(UINT width, UINT height, const uint8_t* bgra,
                         UINT stride);
  ImageButton& preferred_size(float w, float h);
  ImageButton& on_click(ClickHandler handler);

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnMouseEnter(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;

 private:
  void EnsureImage(auralite::Canvas& canvas);

  ClickHandler on_click_;
  float pref_w_ = 48.f;
  float pref_h_ = 48.f;
  bool hovered_ = false;
  bool pressed_ = false;

  auralite::Image image_;
  UINT pixel_w_ = 0;
  UINT pixel_h_ = 0;
  std::vector<uint8_t> pixels_;
  UINT stride_ = 0;
};

}  // namespace auralite::ui
