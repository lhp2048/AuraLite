#pragma once

#include "auralite/ui/node.h"

#include <string>
#include <vector>

namespace auralite::ui {

class ImageView : public Node {
 public:
  // Copies premul BGRA; GPU bitmap is created/recreated on Paint.
  ImageView& SetPixels(UINT width, UINT height, const uint8_t* bgra,
                       UINT stride);
  // Path is loaded lazily on Paint (and after device loss).
  ImageView& LoadFromFile(const std::wstring& path);
  ImageView& preferred_size(float w, float h);

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

 private:
  void EnsureImage(auralite::Canvas& canvas);

  auralite::Image image_;
  UINT pixel_w_ = 0;
  UINT pixel_h_ = 0;
  std::vector<uint8_t> pixels_;
  UINT stride_ = 0;
  std::wstring path_;
  float pref_w_ = 64.f;
  float pref_h_ = 64.f;
};

}  // namespace auralite::ui
