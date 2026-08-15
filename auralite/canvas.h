#pragma once

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <cstdint>
#include <vector>

namespace auralite {

struct ColorF {
  float r = 0.f;
  float g = 0.f;
  float b = 0.f;
  float a = 1.f;

  ColorF() = default;
  ColorF(float rr, float gg, float bb, float aa = 1.f)
      : r(rr), g(gg), b(bb), a(aa) {}

  static ColorF FromRgb(uint8_t rr, uint8_t gg, uint8_t bb, uint8_t aa = 255) {
    return ColorF(rr / 255.f, gg / 255.f, bb / 255.f, aa / 255.f);
  }
};

struct RectF {
  float x = 0.f;
  float y = 0.f;
  float w = 0.f;
  float h = 0.f;
};

enum class TextHAlign { Left, Center, Right };

class Canvas;

// GPU bitmap owned by a Canvas render target. Recreate after device loss.
class Image {
 public:
  Image();
  ~Image();

  Image(const Image&) = delete;
  Image& operator=(const Image&) = delete;

  void Reset();

  // Premultiplied BGRA pixels, top-down.
  bool CreateFromBgra(Canvas& canvas, UINT width, UINT height,
                      const uint8_t* bgra, UINT stride);

  // Load PNG/JPEG/BMP/etc via WIC.
  bool LoadFromFile(Canvas& canvas, const std::wstring& path);

  bool empty() const { return bitmap_ == nullptr; }
  UINT width() const { return width_; }
  UINT height() const { return height_; }
  ID2D1Bitmap* bitmap() const { return bitmap_; }

 private:
  friend class Canvas;
  ID2D1Bitmap* bitmap_ = nullptr;
  UINT width_ = 0;
  UINT height_ = 0;
};

// Direct2D-backed drawing surface bound to an HWND.
class Canvas {
 public:
  Canvas();
  ~Canvas();

  Canvas(const Canvas&) = delete;
  Canvas& operator=(const Canvas&) = delete;

  // Create factories and HWND render target. Safe to call again after device loss.
  bool Init(HWND hwnd);
  void Shutdown();

  // Recreate HWND target if missing (after EndDraw device loss).
  bool EnsureRenderTarget();

  bool BeginDraw();
  // Returns false if the target must be recreated (e.g. device lost).
  bool EndDraw();

  void Resize(UINT width, UINT height);

  void Clear(const ColorF& color);
  void FillRect(const RectF& rect, const ColorF& color);
  void FillRoundedRect(const RectF& rect, float radius_x, float radius_y,
                       const ColorF& color);
  void DrawRect(const RectF& rect, const ColorF& color, float stroke_width = 1.f);
  void FillEllipse(const RectF& rect, const ColorF& color);
  void DrawEllipse(const RectF& rect, const ColorF& color,
                   float stroke_width = 1.f);
  void DrawLine(float x0, float y0, float x1, float y1, const ColorF& color,
                float stroke_width = 1.f);

  // Draws UTF-16 text with DirectWrite (ClearType / grayscale via D2D).
  void DrawText(const std::wstring& text, const RectF& layout_rect,
                const ColorF& color, float font_size = 16.f,
                const wchar_t* font_family = L"Microsoft YaHei UI",
                TextHAlign align = TextHAlign::Left);

  // Width of |text| at |font_size| (empty → 0). Does not require BeginDraw.
  float MeasureTextWidth(const std::wstring& text, float font_size = 16.f,
                         const wchar_t* font_family = L"Microsoft YaHei UI");

  void DrawImage(const Image& image, const RectF& dest);
  void DrawImage(const Image& image, const RectF& src, const RectF& dest);

  // Axis-aligned clip stack (must Pop once per successful Push).
  void PushAxisAlignedClip(const RectF& rect);
  void PopAxisAlignedClip();

  bool is_valid() const { return render_target_ != nullptr; }
  ID2D1HwndRenderTarget* render_target() const { return render_target_; }
  ID2D1Factory* d2d_factory() const { return d2d_factory_; }

  // UI layout / hit-test / paint all use physical pixels (see CreateDeviceResources).
  static constexpr float kUiDpi = 96.f;

 private:
  friend class Image;
  bool CreateDeviceResources();
  void DiscardDeviceResources();
  ID2D1SolidColorBrush* BrushFor(const ColorF& color);

  HWND hwnd_ = nullptr;
  ID2D1Factory* d2d_factory_ = nullptr;
  IDWriteFactory* dwrite_factory_ = nullptr;
  ID2D1HwndRenderTarget* render_target_ = nullptr;
  ID2D1SolidColorBrush* brush_ = nullptr;
};

}  // namespace auralite
