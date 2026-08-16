#include "auralite/canvas.h"

#include <wincodec.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace auralite {
namespace {

template <typename T>
void SafeRelease(T*& ptr) {
  if (ptr) {
    ptr->Release();
    ptr = nullptr;
  }
}

D2D1_COLOR_F ToD2D(const ColorF& c) {
  return D2D1::ColorF(c.r, c.g, c.b, c.a);
}

D2D1_RECT_F ToD2D(const RectF& r) {
  return D2D1::RectF(r.x, r.y, r.x + r.w, r.y + r.h);
}

}  // namespace

Image::Image() = default;

Image::~Image() {
  Reset();
}

void Image::Reset() {
  SafeRelease(bitmap_);
  width_ = 0;
  height_ = 0;
}

bool Image::CreateFromBgra(Canvas& canvas, UINT width, UINT height,
                           const uint8_t* bgra, UINT stride) {
  Reset();
  if (!canvas.EnsureRenderTarget() || !bgra || width == 0 || height == 0) {
    return false;
  }
  const D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
      D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                        D2D1_ALPHA_MODE_PREMULTIPLIED));
  HRESULT hr = canvas.render_target()->CreateBitmap(
      D2D1::SizeU(width, height), bgra, stride, props, &bitmap_);
  if (FAILED(hr)) {
    return false;
  }
  width_ = width;
  height_ = height;
  return true;
}

bool Image::LoadFromFile(Canvas& canvas, const std::wstring& path) {
  Reset();
  if (!canvas.EnsureRenderTarget() || path.empty()) {
    return false;
  }

  IWICImagingFactory* wic = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
  if (FAILED(hr)) {
    return false;
  }

  IWICBitmapDecoder* decoder = nullptr;
  hr = wic->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                      WICDecodeMetadataCacheOnLoad, &decoder);
  if (FAILED(hr)) {
    SafeRelease(wic);
    return false;
  }

  IWICBitmapFrameDecode* frame = nullptr;
  hr = decoder->GetFrame(0, &frame);
  if (FAILED(hr)) {
    SafeRelease(decoder);
    SafeRelease(wic);
    return false;
  }

  IWICFormatConverter* converter = nullptr;
  hr = wic->CreateFormatConverter(&converter);
  if (FAILED(hr)) {
    SafeRelease(frame);
    SafeRelease(decoder);
    SafeRelease(wic);
    return false;
  }

  hr = converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA,
                             WICBitmapDitherTypeNone, nullptr, 0.0,
                             WICBitmapPaletteTypeCustom);
  if (FAILED(hr)) {
    SafeRelease(converter);
    SafeRelease(frame);
    SafeRelease(decoder);
    SafeRelease(wic);
    return false;
  }

  hr = canvas.render_target()->CreateBitmapFromWicBitmap(converter, nullptr,
                                                         &bitmap_);
  if (SUCCEEDED(hr) && bitmap_) {
    const D2D1_SIZE_U size = bitmap_->GetPixelSize();
    width_ = size.width;
    height_ = size.height;
  }

  SafeRelease(converter);
  SafeRelease(frame);
  SafeRelease(decoder);
  SafeRelease(wic);
  return bitmap_ != nullptr;
}

Canvas::Canvas() = default;

void Canvas::SetDpi(float dpi) {
  dpi_ = EffectiveDpi(dpi);
  if (render_target_) {
    render_target_->SetDpi(dpi_, dpi_);
  }
}

Canvas::~Canvas() {
  Shutdown();
}

bool Canvas::Init(HWND hwnd) {
  if (!hwnd) {
    return false;
  }
  hwnd_ = hwnd;

  if (!d2d_factory_) {
    const HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                         &d2d_factory_);
    if (FAILED(hr)) {
      return false;
    }
  }

  if (!dwrite_factory_) {
    const HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&dwrite_factory_));
    if (FAILED(hr)) {
      return false;
    }
  }

  DiscardDeviceResources();
  layered_ = false;
  return CreateDeviceResources();
}

bool Canvas::InitLayered(HWND hwnd) {
  if (!hwnd) {
    return false;
  }
  Shutdown();
  hwnd_ = hwnd;
  layered_ = true;

  if (!d2d_factory_) {
    const HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                         &d2d_factory_);
    if (FAILED(hr)) {
      return false;
    }
  }

  if (!dwrite_factory_) {
    const HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&dwrite_factory_));
    if (FAILED(hr)) {
      return false;
    }
  }

  return CreateDeviceResources();
}

void Canvas::Shutdown() {
  DiscardDeviceResources();
  SafeRelease(dwrite_factory_);
  SafeRelease(d2d_factory_);
  hwnd_ = nullptr;
  layered_ = false;
}

void Canvas::DestroyLayeredSurface() {
  if (dib_dc_ && dib_old_) {
    SelectObject(dib_dc_, dib_old_);
    dib_old_ = nullptr;
  }
  if (dib_bitmap_) {
    DeleteObject(dib_bitmap_);
    dib_bitmap_ = nullptr;
  }
  if (dib_dc_) {
    DeleteDC(dib_dc_);
    dib_dc_ = nullptr;
  }
  dib_bits_ = nullptr;
  dib_w_ = 0;
  dib_h_ = 0;
}

bool Canvas::CreateLayeredSurface(UINT w, UINT h) {
  DestroyLayeredSurface();
  w = w ? w : 1;
  h = h ? h : 1;
  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = static_cast<LONG>(w);
  bmi.bmiHeader.biHeight = -static_cast<LONG>(h);
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  HDC screen = GetDC(nullptr);
  dib_dc_ = CreateCompatibleDC(screen);
  ReleaseDC(nullptr, screen);
  dib_bitmap_ = CreateDIBSection(dib_dc_, &bmi, DIB_RGB_COLORS, &dib_bits_,
                                 nullptr, 0);
  if (!dib_dc_ || !dib_bitmap_ || !dib_bits_) {
    DestroyLayeredSurface();
    return false;
  }
  dib_old_ = static_cast<HBITMAP>(SelectObject(dib_dc_, dib_bitmap_));
  dib_w_ = w;
  dib_h_ = h;
  return true;
}

bool Canvas::EnsureRenderTarget() {
  if (render_target_) {
    return true;
  }
  return CreateDeviceResources();
}

bool Canvas::CreateDeviceResources() {
  if (!d2d_factory_ || !hwnd_) {
    return false;
  }
  if (render_target_) {
    return true;
  }

  RECT rc = {};
  GetClientRect(hwnd_, &rc);

  if (layered_) {
    if (!CreateLayeredSurface(static_cast<UINT>(rc.right),
                              static_cast<UINT>(rc.bottom))) {
      return false;
    }

    const D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                          D2D1_ALPHA_MODE_PREMULTIPLIED),
        dpi_, dpi_);
    ID2D1DCRenderTarget* dc_render_target = nullptr;
    HRESULT hr = d2d_factory_->CreateDCRenderTarget(&props, &dc_render_target);
    if (FAILED(hr)) {
      DestroyLayeredSurface();
      return false;
    }
    render_target_ = dc_render_target;

    const RECT bind = {0, 0, static_cast<LONG>(dib_w_),
                       static_cast<LONG>(dib_h_)};
    hr = dc_render_target->BindDC(dib_dc_, &bind);
    if (FAILED(hr)) {
      DiscardDeviceResources();
      return false;
    }

    render_target_->SetDpi(dpi_, dpi_);
    render_target_->SetTextAntialiasMode(
        D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    hr = render_target_->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Black), &brush_);
    if (FAILED(hr)) {
      DiscardDeviceResources();
      return false;
    }
    return true;
  }

  const D2D1_SIZE_U size = D2D1::SizeU(
      static_cast<UINT32>(rc.right > 0 ? rc.right : 1),
      static_cast<UINT32>(rc.bottom > 0 ? rc.bottom : 1));

  // Layout/hit-test/paint are DIP. Pixel buffer size is GetClientRect.
  // RT DPI is dpi_ (set by Window) so D2D maps DIP drawing to physical pixels.
  const D2D1_RENDER_TARGET_PROPERTIES rt_props = D2D1::RenderTargetProperties(
      D2D1_RENDER_TARGET_TYPE_DEFAULT,
      D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN), dpi_,
      dpi_);
  const D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_props =
      D2D1::HwndRenderTargetProperties(hwnd_, size);

  ID2D1HwndRenderTarget* hwnd_render_target = nullptr;
  HRESULT hr = d2d_factory_->CreateHwndRenderTarget(rt_props, hwnd_props,
                                                    &hwnd_render_target);
  if (FAILED(hr)) {
    return false;
  }
  render_target_ = hwnd_render_target;

  render_target_->SetDpi(dpi_, dpi_);

  hr = render_target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                             &brush_);
  if (FAILED(hr)) {
    DiscardDeviceResources();
    return false;
  }
  return true;
}

void Canvas::DiscardDeviceResources() {
  SafeRelease(brush_);
  SafeRelease(render_target_);
  if (layered_) {
    DestroyLayeredSurface();
  }
}

void Canvas::Resize(UINT width, UINT height) {
  width = width > 0 ? width : 1;
  height = height > 0 ? height : 1;

  if (layered_) {
    if (!EnsureRenderTarget()) {
      return;
    }
    if (width == dib_w_ && height == dib_h_) {
      return;
    }
    if (!CreateLayeredSurface(width, height)) {
      return;
    }
    auto* dc_render_target =
        static_cast<ID2D1DCRenderTarget*>(render_target_);
    if (!dc_render_target) {
      return;
    }
    const RECT bind = {0, 0, static_cast<LONG>(dib_w_),
                       static_cast<LONG>(dib_h_)};
    dc_render_target->BindDC(dib_dc_, &bind);
    render_target_->SetDpi(dpi_, dpi_);
    return;
  }

  if (!EnsureRenderTarget()) {
    return;
  }
  auto* hwnd_render_target =
      static_cast<ID2D1HwndRenderTarget*>(render_target_);
  if (!hwnd_render_target) {
    return;
  }
  hwnd_render_target->Resize(D2D1::SizeU(width, height));
  render_target_->SetDpi(dpi_, dpi_);
}

bool Canvas::BeginDraw() {
  if (!EnsureRenderTarget()) {
    return false;
  }
  if (layered_) {
    auto* dc_render_target =
        static_cast<ID2D1DCRenderTarget*>(render_target_);
    if (dc_render_target && dib_dc_) {
      const RECT bind = {0, 0, static_cast<LONG>(dib_w_),
                         static_cast<LONG>(dib_h_)};
      dc_render_target->BindDC(dib_dc_, &bind);
    }
  }
  render_target_->BeginDraw();
  return true;
}

bool Canvas::EndDraw() {
  if (!render_target_) {
    return false;
  }
  const HRESULT hr = render_target_->EndDraw();
  if (hr == D2DERR_RECREATE_TARGET) {
    DiscardDeviceResources();
    return false;
  }
  if (layered_ && SUCCEEDED(hr) && hwnd_) {
    POINT pt_src = {0, 0};
    SIZE size = {static_cast<LONG>(dib_w_), static_cast<LONG>(dib_h_)};
    POINT pt_dst = {};
    RECT wr = {};
    GetWindowRect(hwnd_, &wr);
    pt_dst.x = wr.left;
    pt_dst.y = wr.top;
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    UpdateLayeredWindow(hwnd_, nullptr, &pt_dst, &size, dib_dc_, &pt_src, 0,
                        &blend, ULW_ALPHA);
  }
  return SUCCEEDED(hr);
}

void Canvas::Clear(const ColorF& color) {
  if (!render_target_) {
    return;
  }
  render_target_->Clear(ToD2D(color));
}

ID2D1SolidColorBrush* Canvas::BrushFor(const ColorF& color) {
  if (!brush_) {
    return nullptr;
  }
  brush_->SetColor(ToD2D(color));
  return brush_;
}

void Canvas::FillRect(const RectF& rect, const ColorF& color) {
  if (!render_target_) {
    return;
  }
  ID2D1SolidColorBrush* brush = BrushFor(color);
  if (!brush) {
    return;
  }
  render_target_->FillRectangle(ToD2D(rect), brush);
}

void Canvas::FillRoundedRect(const RectF& rect, float radius_x, float radius_y,
                             const ColorF& color) {
  if (!render_target_) {
    return;
  }
  ID2D1SolidColorBrush* brush = BrushFor(color);
  if (!brush) {
    return;
  }
  const D2D1_ROUNDED_RECT rr =
      D2D1::RoundedRect(ToD2D(rect), radius_x, radius_y);
  render_target_->FillRoundedRectangle(rr, brush);
}

void Canvas::DrawRect(const RectF& rect, const ColorF& color,
                      float stroke_width) {
  if (!render_target_) {
    return;
  }
  ID2D1SolidColorBrush* brush = BrushFor(color);
  if (!brush) {
    return;
  }
  render_target_->DrawRectangle(ToD2D(rect), brush, stroke_width);
}

void Canvas::DrawDashedRect(const RectF& rect, const ColorF& color,
                            float stroke_width) {
  if (!render_target_ || !d2d_factory_) {
    return;
  }
  ID2D1SolidColorBrush* brush = BrushFor(color);
  if (!brush) {
    return;
  }
  ID2D1StrokeStyle* style = nullptr;
  const HRESULT hr = d2d_factory_->CreateStrokeStyle(
      D2D1::StrokeStyleProperties(
          D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT,
          D2D1_LINE_JOIN_MITER, 10.f, D2D1_DASH_STYLE_DASH, 0.f),
      nullptr, 0, &style);
  if (FAILED(hr) || !style) {
    render_target_->DrawRectangle(ToD2D(rect), brush, stroke_width);
    return;
  }
  render_target_->DrawRectangle(ToD2D(rect), brush, stroke_width, style);
  style->Release();
}

void Canvas::FillEllipse(const RectF& rect, const ColorF& color) {
  if (!render_target_ || rect.w <= 0.f || rect.h <= 0.f) {
    return;
  }
  ID2D1SolidColorBrush* brush = BrushFor(color);
  if (!brush) {
    return;
  }
  const D2D1_ELLIPSE ellipse = D2D1::Ellipse(
      D2D1::Point2F(rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f),
      rect.w * 0.5f, rect.h * 0.5f);
  render_target_->FillEllipse(ellipse, brush);
}

void Canvas::DrawEllipse(const RectF& rect, const ColorF& color,
                         float stroke_width) {
  if (!render_target_ || rect.w <= 0.f || rect.h <= 0.f) {
    return;
  }
  ID2D1SolidColorBrush* brush = BrushFor(color);
  if (!brush) {
    return;
  }
  const D2D1_ELLIPSE ellipse = D2D1::Ellipse(
      D2D1::Point2F(rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f),
      rect.w * 0.5f, rect.h * 0.5f);
  render_target_->DrawEllipse(ellipse, brush, stroke_width);
}

void Canvas::DrawLine(float x0, float y0, float x1, float y1,
                      const ColorF& color, float stroke_width) {
  if (!render_target_) {
    return;
  }
  ID2D1SolidColorBrush* brush = BrushFor(color);
  if (!brush) {
    return;
  }
  render_target_->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush,
                           stroke_width);
}

void Canvas::PushAxisAlignedClip(const RectF& rect) {
  if (!render_target_) {
    return;
  }
  render_target_->PushAxisAlignedClip(ToD2D(rect),
                                      D2D1_ANTIALIAS_MODE_ALIASED);
}

void Canvas::PopAxisAlignedClip() {
  if (!render_target_) {
    return;
  }
  render_target_->PopAxisAlignedClip();
}

void Canvas::DrawText(const std::wstring& text, const RectF& layout_rect,
                      const ColorF& color, float font_size,
                      const wchar_t* font_family, TextHAlign align) {
  if (!render_target_ || !dwrite_factory_ || text.empty()) {
    return;
  }

  IDWriteTextFormat* format = nullptr;
  HRESULT hr = dwrite_factory_->CreateTextFormat(
      font_family ? font_family : L"Microsoft YaHei UI", nullptr,
      DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL, font_size, L"zh-cn", &format);
  if (FAILED(hr) || !format) {
    return;
  }

  DWRITE_TEXT_ALIGNMENT dwrite_align = DWRITE_TEXT_ALIGNMENT_LEADING;
  switch (align) {
    case TextHAlign::Center:
      dwrite_align = DWRITE_TEXT_ALIGNMENT_CENTER;
      break;
    case TextHAlign::Right:
      dwrite_align = DWRITE_TEXT_ALIGNMENT_TRAILING;
      break;
    case TextHAlign::Left:
    default:
      dwrite_align = DWRITE_TEXT_ALIGNMENT_LEADING;
      break;
  }
  format->SetTextAlignment(dwrite_align);
  format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
  format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

  ID2D1SolidColorBrush* brush = BrushFor(color);
  if (brush) {
    render_target_->DrawText(text.c_str(), static_cast<UINT32>(text.size()),
                             format, ToD2D(layout_rect), brush,
                             D2D1_DRAW_TEXT_OPTIONS_CLIP);
  }
  format->Release();
}

float Canvas::MeasureTextWidth(const std::wstring& text, float font_size,
                               const wchar_t* font_family) {
  return MeasureUiTextWidth(text, font_size, font_family);
}

float MeasureUiTextWidth(const std::wstring& text, float font_size,
                         const wchar_t* font_family) {
  if (text.empty()) {
    return 0.f;
  }

  static IDWriteFactory* factory = nullptr;
  if (!factory) {
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                        reinterpret_cast<IUnknown**>(&factory));
  }
  if (!factory) {
    // Fallback: CJK ≈ em square; Latin ≈ 0.55em (old Latin-only heuristic).
    float w = 0.f;
    for (wchar_t ch : text) {
      w += (ch > 0x7F) ? font_size : (font_size * 0.55f);
    }
    return w;
  }

  IDWriteTextFormat* format = nullptr;
  HRESULT hr = factory->CreateTextFormat(
      font_family ? font_family : L"Microsoft YaHei UI", nullptr,
      DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL, font_size, L"zh-cn", &format);
  if (FAILED(hr) || !format) {
    float w = 0.f;
    for (wchar_t ch : text) {
      w += (ch > 0x7F) ? font_size : (font_size * 0.55f);
    }
    return w;
  }

  IDWriteTextLayout* layout = nullptr;
  hr = factory->CreateTextLayout(text.c_str(),
                                 static_cast<UINT32>(text.size()), format,
                                 100000.f, font_size * 2.f, &layout);
  format->Release();
  if (FAILED(hr) || !layout) {
    float w = 0.f;
    for (wchar_t ch : text) {
      w += (ch > 0x7F) ? font_size : (font_size * 0.55f);
    }
    return w;
  }

  DWRITE_TEXT_METRICS metrics = {};
  hr = layout->GetMetrics(&metrics);
  layout->Release();
  if (FAILED(hr)) {
    float w = 0.f;
    for (wchar_t ch : text) {
      w += (ch > 0x7F) ? font_size : (font_size * 0.55f);
    }
    return w;
  }
  return metrics.widthIncludingTrailingWhitespace;
}

void Canvas::DrawImage(const Image& image, const RectF& dest) {
  if (!render_target_ || image.empty()) {
    return;
  }
  render_target_->DrawBitmap(image.bitmap(), ToD2D(dest));
}

void Canvas::DrawImage(const Image& image, const RectF& src,
                       const RectF& dest) {
  if (!render_target_ || image.empty()) {
    return;
  }
  render_target_->DrawBitmap(image.bitmap(), ToD2D(dest), 1.f,
                             D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                             ToD2D(src));
}

}  // namespace auralite
