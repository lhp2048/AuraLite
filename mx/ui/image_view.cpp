#include "mx/ui/image_view.h"

#include "mx/ui/theme.h"

namespace mx::ui {

ImageView::ImageView() {
  fixed_width(64.f);
  fixed_height(64.f);
}

ImageView& ImageView::SetPixels(UINT width, UINT height, const uint8_t* bgra,
                                UINT stride) {
  image_.Reset();
  path_.clear();
  pixel_w_ = width;
  pixel_h_ = height;
  stride_ = stride;
  pixels_.assign(bgra, bgra + stride * height);
  if (preferred_width() <= 0.f || preferred_height() <= 0.f) {
    fixed_width(static_cast<float>(width));
    fixed_height(static_cast<float>(height));
  }
  return *this;
}

ImageView& ImageView::LoadFromFile(const std::wstring& path) {
  image_.Reset();
  pixels_.clear();
  pixel_w_ = pixel_h_ = 0;
  stride_ = 0;
  path_ = path;
  return *this;
}

ImageView& ImageView::preferred_size(float w, float h) {
  fixed_width(w);
  fixed_height(h);
  return *this;
}

SizeF ImageView::Measure(float max_w, float max_h) {
  const float hug_w = preferred_width() > 0.f ? preferred_width() : 64.f;
  const float hug_h = preferred_height() > 0.f ? preferred_height() : 64.f;
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void ImageView::EnsureImage(mx::Canvas& canvas) {
  if (!image_.empty()) {
    return;
  }
  if (!path_.empty()) {
    image_.LoadFromFile(canvas, path_);
    return;
  }
  if (!pixels_.empty() && pixel_w_ > 0 && pixel_h_ > 0) {
    image_.CreateFromBgra(canvas, pixel_w_, pixel_h_, pixels_.data(), stride_);
  }
}

void ImageView::Paint(mx::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  EnsureImage(canvas);
  if (image_.empty()) {
    canvas.FillRect(bounds_, Theme::Active().surface_alt);
    return;
  }
  canvas.DrawImage(image_, bounds_);
}

void ImageView::OnDeviceLost() {
  image_.Reset();
  Node::OnDeviceLost();
}

}  // namespace mx::ui
