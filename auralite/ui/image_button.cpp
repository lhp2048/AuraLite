#include "auralite/ui/image_button.h"

#include <algorithm>

namespace auralite::ui {

ImageButton& ImageButton::SetPixels(UINT width, UINT height,
                                    const uint8_t* bgra, UINT stride) {
  image_.Reset();
  pixel_w_ = width;
  pixel_h_ = height;
  stride_ = stride;
  pixels_.assign(bgra, bgra + stride * height);
  return *this;
}

ImageButton& ImageButton::preferred_size(float w, float h) {
  pref_w_ = w;
  pref_h_ = h;
  return *this;
}

ImageButton& ImageButton::on_click(ClickHandler handler) {
  on_click_ = std::move(handler);
  return *this;
}

SizeF ImageButton::Measure(float /*max_w*/, float /*max_h*/) {
  return SizeF{pref_w_, pref_h_};
}

void ImageButton::EnsureImage(auralite::Canvas& canvas) {
  if (!image_.empty() || pixels_.empty() || pixel_w_ == 0 || pixel_h_ == 0) {
    return;
  }
  image_.CreateFromBgra(canvas, pixel_w_, pixel_h_, pixels_.data(), stride_);
}

void ImageButton::Paint(auralite::Canvas& canvas) {
  EnsureImage(canvas);

  ColorF chrome = ColorF::FromRgb(230, 235, 242);
  if (pressed_) {
    chrome = ColorF::FromRgb(190, 200, 215);
  } else if (hovered_) {
    chrome = ColorF::FromRgb(210, 220, 232);
  }
  canvas.FillRoundedRect(bounds_, 8.f, 8.f, chrome);

  if (!image_.empty()) {
    const float pad = 6.f;
    const RectF dest{bounds_.x + pad, bounds_.y + pad,
                     std::max(0.f, bounds_.w - pad * 2.f),
                     std::max(0.f, bounds_.h - pad * 2.f)};
    canvas.DrawImage(image_, dest);
  }
}

void ImageButton::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  pressed_ = true;
}

void ImageButton::OnMouseUp(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  const bool was_pressed = pressed_;
  pressed_ = false;
  if (was_pressed && ContainsPoint(bounds_, e.x, e.y) && on_click_) {
    on_click_();
  }
}

void ImageButton::OnMouseEnter(const MouseEvent&) {
  hovered_ = true;
}

void ImageButton::OnMouseLeave(const MouseEvent&) {
  hovered_ = false;
  pressed_ = false;
}

void ImageButton::OnDeviceLost() {
  image_.Reset();
  Node::OnDeviceLost();
}

}  // namespace auralite::ui
