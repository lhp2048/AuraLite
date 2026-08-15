#include "auralite/ui/image_view.h"

namespace auralite::ui {

ImageView& ImageView::SetPixels(UINT width, UINT height, const uint8_t* bgra,
                                UINT stride) {
  image_.Reset();
  path_.clear();
  pixel_w_ = width;
  pixel_h_ = height;
  stride_ = stride;
  pixels_.assign(bgra, bgra + stride * height);
  if (pref_w_ <= 0.f || pref_h_ <= 0.f) {
    pref_w_ = static_cast<float>(width);
    pref_h_ = static_cast<float>(height);
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
  pref_w_ = w;
  pref_h_ = h;
  return *this;
}

SizeF ImageView::Measure(float /*max_w*/, float /*max_h*/) {
  return SizeF{pref_w_, pref_h_};
}

void ImageView::EnsureImage(auralite::Canvas& canvas) {
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

void ImageView::Paint(auralite::Canvas& canvas) {
  EnsureImage(canvas);
  if (image_.empty()) {
    canvas.FillRect(bounds_, ColorF::FromRgb(210, 216, 224));
    return;
  }
  canvas.DrawImage(image_, bounds_);
}

}  // namespace auralite::ui
