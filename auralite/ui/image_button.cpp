#include "auralite/ui/image_button.h"

#include <algorithm>

namespace auralite::ui {

ImageButton::ImageButton() {
  set_focusable(true);
  fixed_width(48.f);
  fixed_height(48.f);
}

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
  fixed_width(w);
  fixed_height(h);
  return *this;
}

ImageButton& ImageButton::on_click(ClickHandler handler) {
  on_click_ = std::move(handler);
  return *this;
}

SizeF ImageButton::Measure(float max_w, float max_h) {
  const float hug_w = preferred_width() > 0.f ? preferred_width() : 48.f;
  const float hug_h = preferred_height() > 0.f ? preferred_height() : 48.f;
  return ResolveSize(max_w, max_h, hug_w, hug_h);
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
