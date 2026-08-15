
#include "canvas_d2d.h"

#include <algorithm>
#include <cstring>
#include <vector>
#include <wincodec.h>

#include "base/logging.h"

#include "bitmap.h"
#include "brush.h"
#include "color.h"
#include "dwrite_text.h"
#include "font.h"
#include "rect.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace
{

    template<typename T>
    void SafeRelease(T*& ptr)
    {
        if(ptr)
        {
            ptr->Release();
            ptr = NULL;
        }
    }

    void SetTextAntialiasForTarget(ID2D1RenderTarget* rt)
    {
        if(!rt)
        {
            return;
        }
        const D2D1_PIXEL_FORMAT pf = rt->GetPixelFormat();
        if(pf.alphaMode==D2D1_ALPHA_MODE_IGNORE ||
            pf.alphaMode==D2D1_ALPHA_MODE_UNKNOWN)
        {
            rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        }
        else
        {
            // Premultiplied offscreen targets cannot use ClearType.
            rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        }
    }

}

namespace gfx
{

    CanvasD2D::CanvasD2D()
        : d2d_factory_(NULL),
          rt_(NULL),
          wic_bitmap_(NULL),
          brush_(NULL),
          cached_bitmap_(NULL),
          cached_pb_(NULL),
          cached_pixels_(NULL),
          cached_bw_(0),
          cached_bh_(0),
          cached_stride_(0),
          recycled_layer_(NULL),
          platform_dc_(NULL),
          platform_dib_(NULL),
          platform_old_(NULL),
          platform_bits_(NULL),
          width_(0),
          height_(0),
          is_opaque_(false),
          drawing_(false),
          clip_depth_(0),
          layer_depth_(0) {}

    CanvasD2D::CanvasD2D(int width, int height, bool is_opaque)
        : d2d_factory_(NULL),
          rt_(NULL),
          wic_bitmap_(NULL),
          brush_(NULL),
          cached_bitmap_(NULL),
          cached_pb_(NULL),
          cached_pixels_(NULL),
          cached_bw_(0),
          cached_bh_(0),
          cached_stride_(0),
          recycled_layer_(NULL),
          platform_dc_(NULL),
          platform_dib_(NULL),
          platform_old_(NULL),
          platform_bits_(NULL),
          width_(0),
          height_(0),
          is_opaque_(false),
          drawing_(false),
          clip_depth_(0),
          layer_depth_(0)
    {
        const bool initialized = initialize(width, height, is_opaque);
        DCHECK(initialized);
    }

    CanvasD2D::~CanvasD2D()
    {
        DiscardResources();
    }

    bool CanvasD2D::initialize(int width, int height, bool is_opaque)
    {
        FlushDrawState();
        DiscardDeviceResources();

        if(width<=0)
        {
            width = 1;
        }
        if(height<=0)
        {
            height = 1;
        }
        width_ = width;
        height_ = height;
        is_opaque_ = is_opaque;

        if(!d2d_factory_)
        {
            const HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                &d2d_factory_);
            if(FAILED(hr) || !d2d_factory_)
            {
                return false;
            }
        }

        IWICImagingFactory* wic = NULL;
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
        if(hr==CO_E_NOTINITIALIZED)
        {
            CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            hr = CoCreateInstance(CLSID_WICImagingFactory, NULL,
                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
        }
        if(FAILED(hr) || !wic)
        {
            return false;
        }

        const WICPixelFormatGUID wic_format = is_opaque ?
            GUID_WICPixelFormat32bppBGR : GUID_WICPixelFormat32bppPBGRA;
        hr = wic->CreateBitmap(static_cast<UINT>(width),
            static_cast<UINT>(height), wic_format,
            WICBitmapCacheOnLoad, &wic_bitmap_);
        wic->Release();
        if(FAILED(hr) || !wic_bitmap_)
        {
            wic_bitmap_ = NULL;
            return false;
        }

        // 96 DPI so 1 DIP == 1 bitmap pixel.
        const D2D1_PIXEL_FORMAT pixel_format = D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            is_opaque ? D2D1_ALPHA_MODE_IGNORE : D2D1_ALPHA_MODE_PREMULTIPLIED);
        const D2D1_RENDER_TARGET_PROPERTIES rt_props =
            D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
                pixel_format, 96.0f, 96.0f);

        hr = d2d_factory_->CreateWicBitmapRenderTarget(wic_bitmap_, rt_props,
            &rt_);
        if(FAILED(hr) || !rt_)
        {
            SafeRelease(wic_bitmap_);
            rt_ = NULL;
            return false;
        }

        hr = rt_->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::Black), &brush_);
        if(FAILED(hr) || !brush_)
        {
            DiscardDeviceResources();
            return false;
        }

        SetTextAntialiasForTarget(rt_);
        return true;
    }

    bool CanvasD2D::BeginDraw()
    {
        if(!rt_)
        {
            return false;
        }
        if(drawing_)
        {
            return true;
        }
        rt_->BeginDraw();
        drawing_ = true;
        return true;
    }

    bool CanvasD2D::EndDraw()
    {
        if(!rt_ || !drawing_)
        {
            return false;
        }
        PopRemainingLayers();
        const HRESULT hr = rt_->EndDraw();
        drawing_ = false;
        if(hr==D2DERR_RECREATE_TARGET)
        {
            const int w = width_;
            const int h = height_;
            const bool opaque = is_opaque_;
            DiscardDeviceResources();
            initialize(w, h, opaque);
            return false;
        }
        return SUCCEEDED(hr);
    }

    void CanvasD2D::Clear(const Color& color)
    {
        if(!rt_)
        {
            return;
        }
        // ID2D1RenderTarget::Clear ignores clip and layers. Skipping Clear inside
        // a layer leaves the destination intact while PushLayer still applies
        // hover opacity to subsequent draws.
        if(layer_depth_>0)
        {
            return;
        }
        EnsureDrawing();
        rt_->Clear(ToD2DColor(color));
    }

    void CanvasD2D::Save()
    {
        if(!rt_ || !d2d_factory_)
        {
            return;
        }
        EnsureDrawing();

        SavedState state;
        state.block = NULL;
        state.clip_depth = clip_depth_;
        d2d_factory_->CreateDrawingStateBlock(&state.block);
        if(state.block)
        {
            rt_->SaveDrawingState(state.block);
        }
        states_.push(state);
    }

    void CanvasD2D::Restore()
    {
        if(states_.empty() || !rt_)
        {
            NOTREACHED();
            return;
        }

        const SavedState state = states_.top();
        states_.pop();
        while(clip_depth_>state.clip_depth)
        {
            rt_->PopAxisAlignedClip();
            --clip_depth_;
        }
        if(state.block)
        {
            rt_->RestoreDrawingState(state.block);
            state.block->Release();
        }
    }

    void CanvasD2D::SaveLayer(uint8 alpha)
    {
        PushOpacityLayer(alpha, NULL);
    }

    void CanvasD2D::SaveLayer(uint8 alpha, const Rect& layer_bounds)
    {
        const D2D1_RECT_F bounds = ToD2DRect(layer_bounds.x(), layer_bounds.y(),
            layer_bounds.width(), layer_bounds.height());
        PushOpacityLayer(alpha, &bounds);
    }

    void CanvasD2D::RestoreLayer()
    {
        if(layer_depth_==0)
        {
            return;
        }
        --layer_depth_;
        if(layers_.empty())
        {
            return;
        }
        ID2D1Layer* layer = layers_.top();
        layers_.pop();
        if(rt_ && drawing_ && layer)
        {
            rt_->PopLayer();
        }
        SafeRelease(layer);
    }

    bool CanvasD2D::ClipRectInt(int x, int y, int w, int h)
    {
        if(!rt_)
        {
            return false;
        }
        EnsureDrawing();
        rt_->PushAxisAlignedClip(
            ToD2DRect(x, y, w, h), D2D1_ANTIALIAS_MODE_ALIASED);
        ++clip_depth_;
        // true = non-empty clip, matching View::ProcessPaint.
        return w>0 && h>0;
    }

    void CanvasD2D::TranslateInt(int x, int y)
    {
        if(!rt_)
        {
            return;
        }
        EnsureDrawing();
        D2D1_MATRIX_3X2_F current;
        rt_->GetTransform(&current);
        rt_->SetTransform(
            current * D2D1::Matrix3x2F::Translation(
                static_cast<FLOAT>(x), static_cast<FLOAT>(y)));
    }

    void CanvasD2D::ScaleInt(int x, int y)
    {
        if(!rt_)
        {
            return;
        }
        EnsureDrawing();
        D2D1_MATRIX_3X2_F current;
        rt_->GetTransform(&current);
        rt_->SetTransform(
            current * D2D1::Matrix3x2F::Scale(
                static_cast<FLOAT>(x), static_cast<FLOAT>(y)));
    }

    void CanvasD2D::FillRectInt(const Color& color, int x, int y, int w, int h)
    {
        if(!rt_ || w<=0 || h<=0)
        {
            return;
        }
        EnsureDrawing();
        ID2D1SolidColorBrush* brush = BrushFor(color);
        if(!brush)
        {
            return;
        }
        rt_->FillRectangle(ToD2DRect(x, y, w, h), brush);
    }

    void CanvasD2D::FillEllipseInt(const Color& color, int x, int y, int w, int h)
    {
        if(!rt_ || w<=0 || h<=0)
        {
            return;
        }
        EnsureDrawing();
        ID2D1SolidColorBrush* brush = BrushFor(color);
        if(!brush)
        {
            return;
        }
        const D2D1_ELLIPSE ellipse = D2D1::Ellipse(
            D2D1::Point2F(x + w * 0.5f, y + h * 0.5f),
            w * 0.5f, h * 0.5f);
        rt_->FillEllipse(ellipse, brush);
    }

    void CanvasD2D::DrawEllipseInt(const Color& color, int x, int y, int w, int h)
    {
        if(!rt_ || w<=0 || h<=0)
        {
            return;
        }
        EnsureDrawing();
        ID2D1SolidColorBrush* brush = BrushFor(color);
        if(!brush)
        {
            return;
        }
        const D2D1_ELLIPSE ellipse = D2D1::Ellipse(
            D2D1::Point2F(x + w * 0.5f, y + h * 0.5f),
            w * 0.5f, h * 0.5f);
        rt_->DrawEllipse(ellipse, brush, 1.f);
    }

    void CanvasD2D::FillRoundedRectInt(const Color& color,
        int x, int y, int w, int h, int radius)
    {
        if(!rt_ || w<=0 || h<=0)
        {
            return;
        }
        EnsureDrawing();
        ID2D1SolidColorBrush* brush = BrushFor(color);
        if(!brush)
        {
            return;
        }
        float r = static_cast<float>(std::max(0, radius));
        const float max_r = std::min(w, h) * 0.5f;
        if(r > max_r)
        {
            r = max_r;
        }
        const D2D1_ROUNDED_RECT rounded = D2D1::RoundedRect(
            ToD2DRect(x, y, w, h), r, r);
        rt_->FillRoundedRectangle(rounded, brush);
    }

    void CanvasD2D::FillRectInt(const Brush& brush,
        int x, int y, int w, int h)
    {
        if(!rt_ || w<=0 || h<=0)
        {
            return;
        }
        if(brush.type()==Brush::SOLID)
        {
            FillRectInt(brush.color(), x, y, w, h);
            return;
        }
        if(brush.type()!=Brush::LINEAR_GRADIENT)
        {
            return;
        }

        EnsureDrawing();
        D2D1_GRADIENT_STOP stops[2];
        stops[0].position = 0.0f;
        stops[0].color = ToD2DColor(brush.color1());
        stops[1].position = 1.0f;
        stops[1].color = ToD2DColor(brush.color2());
        ID2D1GradientStopCollection* collection = NULL;
        HRESULT hr = rt_->CreateGradientStopCollection(stops, 2, &collection);
        if(FAILED(hr) || !collection)
        {
            return;
        }
        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props = brush.horizontal()
            ? D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(y)),
                D2D1::Point2F(static_cast<FLOAT>(x+w), static_cast<FLOAT>(y)))
            : D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(y)),
                D2D1::Point2F(static_cast<FLOAT>(x),
                    static_cast<FLOAT>(y+h)));
        ID2D1LinearGradientBrush* lg_brush = NULL;
        hr = rt_->CreateLinearGradientBrush(props, collection, &lg_brush);
        collection->Release();
        if(FAILED(hr) || !lg_brush)
        {
            return;
        }
        rt_->FillRectangle(ToD2DRect(x, y, w, h), lg_brush);
        lg_brush->Release();
    }

    void CanvasD2D::DrawRectInt(const Color& color, int x, int y, int w, int h)
    {
        if(!rt_ || w<=0 || h<=0)
        {
            return;
        }
        EnsureDrawing();
        ID2D1SolidColorBrush* brush = BrushFor(color);
        if(!brush)
        {
            return;
        }
        rt_->DrawRectangle(ToD2DRect(x, y, w, h), brush, 1.0f);
    }

    void CanvasD2D::DrawLineInt(const Color& color,
        int x1, int y1, int x2, int y2)
    {
        if(!rt_)
        {
            return;
        }
        EnsureDrawing();
        ID2D1SolidColorBrush* brush = BrushFor(color);
        if(!brush)
        {
            return;
        }
        rt_->DrawLine(
            D2D1::Point2F(static_cast<FLOAT>(x1), static_cast<FLOAT>(y1)),
            D2D1::Point2F(static_cast<FLOAT>(x2), static_cast<FLOAT>(y2)),
            brush, 1.0f);
    }

    void CanvasD2D::DrawBitmapInt(const Bitmap& bitmap, int x, int y)
    {
        if(bitmap.IsNull())
        {
            return;
        }
        DrawBitmapInt(bitmap, 0, 0, bitmap.Width(), bitmap.Height(),
            x, y, bitmap.Width(), bitmap.Height());
    }

    void CanvasD2D::DrawBitmapInt(const Bitmap& bitmap,
        int src_x, int src_y, int src_w, int src_h,
        int dest_x, int dest_y, int dest_w, int dest_h)
    {
        if(!rt_ || bitmap.IsNull() || src_w<=0 || src_h<=0 ||
            dest_w<=0 || dest_h<=0)
        {
            return;
        }
        const uint8* pixels = bitmap.GetPixels();
        const int stride = bitmap.Stride();
        if(!pixels || stride<=0)
        {
            return;
        }

        const int bw = bitmap.Width();
        const int bh = bitmap.Height();
        if(bw<=0 || bh<=0)
        {
            return;
        }

        EnsureDrawing();
        ID2D1Bitmap* d2d_bitmap = GetOrCreateD2DBitmap(bitmap);
        if(!d2d_bitmap)
        {
            return;
        }

        rt_->DrawBitmap(d2d_bitmap,
            ToD2DRect(dest_x, dest_y, dest_w, dest_h),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            ToD2DRect(src_x, src_y, src_w, src_h));
    }

    void CanvasD2D::DrawStringInt(const std::wstring& text,
        const Font& font,
        const Color& color,
        int x, int y, int w, int h)
    {
        DrawStringInt(text, font, color, x, y, w, h, 0);
    }

    void CanvasD2D::DrawStringInt(const std::wstring& text,
        const Font& font,
        const Color& color,
        const Rect& display_rect)
    {
        DrawStringInt(text, font, color, display_rect.x(), display_rect.y(),
            display_rect.width(), display_rect.height());
    }

    void CanvasD2D::DrawStringInt(const std::wstring& text,
        const Font& font,
        const Color& color,
        int x, int y, int w, int h,
        int flags)
    {
        if(!rt_ || text.empty() || w<=0 || h<=0)
        {
            return;
        }
        EnsureDrawing();
        SetTextAntialiasForTarget(rt_);

        IDWriteTextLayout* layout = dwrite_text::CreateLayout(text, font, flags,
            static_cast<float>(w), static_cast<float>(h));
        if(!layout)
        {
            return;
        }

        ID2D1SolidColorBrush* brush = BrushFor(color);
        if(brush)
        {
            rt_->DrawTextLayout(
                D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(y)),
                layout,
                brush,
                D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
        layout->Release();
    }

    void CanvasD2D::DrawFocusRect(int x, int y, int width, int height)
    {
        DrawRectInt(Color(128, 128, 128), x, y, width, height);
    }

    void CanvasD2D::TileImageInt(const Bitmap& bitmap,
        int x, int y, int w, int h)
    {
        TileImageInt(bitmap, 0, 0, x, y, w, h);
    }

    void CanvasD2D::TileImageInt(const Bitmap& bitmap,
        int src_x, int src_y,
        int dest_x, int dest_y, int w, int h)
    {
        if(bitmap.IsNull() || w<=0 || h<=0)
        {
            return;
        }
        const int tile_w = bitmap.Width() - src_x;
        const int tile_h = bitmap.Height() - src_y;
        if(tile_w<=0 || tile_h<=0)
        {
            return;
        }
        for(int ty=0; ty<h; ty+=tile_h)
        {
            const int dh = (ty+tile_h>h) ? (h-ty) : tile_h;
            for(int tx=0; tx<w; tx+=tile_w)
            {
                const int dw = (tx+tile_w>w) ? (w-tx) : tile_w;
                DrawBitmapInt(bitmap, src_x, src_y, dw, dh,
                    dest_x+tx, dest_y+ty, dw, dh);
            }
        }
    }

    void CanvasD2D::DrawToHDC(HDC hdc, int x, int y, const RECT* src_rect)
    {
        int src_x = 0;
        int src_y = 0;
        int width = width_;
        int height = height_;
        if(src_rect)
        {
            src_x = src_rect->left;
            src_y = src_rect->top;
            width = src_rect->right - src_rect->left;
            height = src_rect->bottom - src_rect->top;
        }
        CopyToHdc(hdc, x, y, src_x, src_y, width, height);
    }

    HDC CanvasD2D::BeginPlatformPaint()
    {
        ReleasePlatformDc();
        if(!CopyToHdc(NULL, 0, 0, 0, 0, width_, height_))
        {
            return NULL;
        }
        return platform_dc_;
    }

    void CanvasD2D::EndPlatformPaint(HDC /*dc*/)
    {
        // Must finish any open BeginDraw before tearing down the RT. Mid-paint
        // writeback used to leave drawing_=true with a fresh RT that never got
        // BeginDraw, so all later FillRect/DrawString became no-ops.
        const bool was_drawing = drawing_;
        if(drawing_)
        {
            EndDraw();
        }

        // GDI callers draw into the platform DIB; push those pixels back into
        // the WIC bitmap so subsequent D2D paints see them (FamilyShell icons).
        if(platform_bits_ && wic_bitmap_)
        {
            CopyPlatformDcToBitmap();
            RecreateRenderTargetFromBitmap();
            if(was_drawing && rt_)
            {
                // Resume drawing on the recreated target. Callers that used
                // BeginPlatformPaint outside of an active draw still finish
                // with drawing_ == false (was_drawing was false).
                BeginDraw();
            }
        }
        ReleasePlatformDc();
    }

    CanvasD2D* CanvasD2D::AsCanvasD2D()
    {
        return this;
    }

    const CanvasD2D* CanvasD2D::AsCanvasD2D() const
    {
        return this;
    }

    void CanvasD2D::EnsureDrawing()
    {
        BeginDraw();
    }

    void CanvasD2D::FlushDrawState()
    {
        if(rt_ && drawing_)
        {
            PopRemainingLayers();
            while(clip_depth_>0)
            {
                rt_->PopAxisAlignedClip();
                --clip_depth_;
            }
            rt_->EndDraw();
            drawing_ = false;
        }
        while(!states_.empty())
        {
            SavedState state = states_.top();
            states_.pop();
            if(state.block)
            {
                state.block->Release();
            }
        }
        clip_depth_ = 0;
        layer_depth_ = 0;
    }

    void CanvasD2D::PushOpacityLayer(uint8 alpha, const D2D1_RECT_F* content_bounds)
    {
        ID2D1Layer* layer = NULL;
        if(rt_)
        {
            EnsureDrawing();
            if(recycled_layer_)
            {
                layer = recycled_layer_;
                recycled_layer_ = NULL;
            }
            else
            {
                const HRESULT hr = rt_->CreateLayer(&layer);
                if(FAILED(hr) || !layer)
                {
                    layer = NULL;
                }
            }
            if(layer)
            {
                const D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters(
                    content_bounds ? *content_bounds : D2D1::InfiniteRect(),
                    NULL,
                    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                    D2D1::IdentityMatrix(),
                    static_cast<FLOAT>(alpha) / 255.0f,
                    NULL,
                    D2D1_LAYER_OPTIONS_NONE);
                rt_->PushLayer(params, layer);
            }
        }
        layers_.push(layer);
        ++layer_depth_;
    }

    void CanvasD2D::PopRemainingLayers()
    {
        while(!layers_.empty())
        {
            ID2D1Layer* layer = layers_.top();
            layers_.pop();
            if(rt_ && drawing_ && layer)
            {
                rt_->PopLayer();
            }
            if(layer)
            {
                if(!recycled_layer_)
                {
                    recycled_layer_ = layer;
                }
                else
                {
                    layer->Release();
                }
            }
        }
        layer_depth_ = 0;
    }

    void CanvasD2D::DiscardDeviceResources()
    {
        ReleasePlatformDc();
        InvalidateBitmapCache();
        SafeRelease(recycled_layer_);
        SafeRelease(brush_);
        SafeRelease(rt_);
        SafeRelease(wic_bitmap_);
    }

    void CanvasD2D::DiscardResources()
    {
        FlushDrawState();
        DiscardDeviceResources();
        SafeRelease(d2d_factory_);
    }

    void CanvasD2D::ReleasePlatformDc()
    {
        if(platform_dc_)
        {
            if(platform_old_)
            {
                SelectObject(platform_dc_, platform_old_);
                platform_old_ = NULL;
            }
            DeleteDC(platform_dc_);
            platform_dc_ = NULL;
        }
        if(platform_dib_)
        {
            DeleteObject(platform_dib_);
            platform_dib_ = NULL;
        }
        platform_bits_ = NULL;
    }

    bool CanvasD2D::CopyPlatformDcToBitmap()
    {
        if(!platform_bits_ || !wic_bitmap_ || width_<=0 || height_<=0)
        {
            return false;
        }

        // Must not hold a WIC RT while locking the bitmap for write.
        SafeRelease(brush_);
        SafeRelease(rt_);

        WICRect lock_rect = { 0, 0, width_, height_ };
        IWICBitmapLock* lock = NULL;
        HRESULT hr = wic_bitmap_->Lock(&lock_rect, WICBitmapLockWrite, &lock);
        if(FAILED(hr) || !lock)
        {
            return false;
        }

        UINT buf_size = 0;
        BYTE* data = NULL;
        hr = lock->GetDataPointer(&buf_size, &data);
        UINT stride = 0;
        if(SUCCEEDED(hr))
        {
            hr = lock->GetStride(&stride);
        }
        if(FAILED(hr) || !data || stride==0)
        {
            lock->Release();
            return false;
        }

        const BYTE* src = static_cast<const BYTE*>(platform_bits_);
        const size_t row_bytes = static_cast<size_t>(width_)*4;
        for(int row=0; row<height_; ++row)
        {
            BYTE* dst_row = data + static_cast<size_t>(row)*stride;
            memcpy(dst_row, src + static_cast<size_t>(row)*row_bytes, row_bytes);
            // GDI FillRect/BitBlt leaves alpha=0 on BI_RGB DIBs; opaque canvases
            // need solid alpha or layered/composited output vanishes.
            if(is_opaque_)
            {
                for(int col=0; col<width_; ++col)
                {
                    dst_row[col*4 + 3] = 0xFF;
                }
            }
        }
        lock->Release();
        return true;
    }

    bool CanvasD2D::RecreateRenderTargetFromBitmap()
    {
        if(!d2d_factory_ || !wic_bitmap_)
        {
            return false;
        }
        InvalidateBitmapCache();
        SafeRelease(recycled_layer_);
        SafeRelease(brush_);
        SafeRelease(rt_);

        const D2D1_PIXEL_FORMAT pixel_format = D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            is_opaque_ ? D2D1_ALPHA_MODE_IGNORE : D2D1_ALPHA_MODE_PREMULTIPLIED);
        const D2D1_RENDER_TARGET_PROPERTIES rt_props =
            D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
                pixel_format, 96.0f, 96.0f);
        HRESULT hr = d2d_factory_->CreateWicBitmapRenderTarget(wic_bitmap_,
            rt_props, &rt_);
        if(FAILED(hr) || !rt_)
        {
            rt_ = NULL;
            return false;
        }
        hr = rt_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush_);
        if(FAILED(hr) || !brush_)
        {
            SafeRelease(rt_);
            return false;
        }
        SetTextAntialiasForTarget(rt_);
        return true;
    }

    void CanvasD2D::InvalidateBitmapCache()
    {
        SafeRelease(cached_bitmap_);
        cached_pb_ = NULL;
        cached_pixels_ = NULL;
        cached_bw_ = 0;
        cached_bh_ = 0;
        cached_stride_ = 0;
    }

    ID2D1Bitmap* CanvasD2D::GetOrCreateD2DBitmap(const Bitmap& bitmap)
    {
        if(!rt_ || bitmap.IsNull())
        {
            return NULL;
        }
        const uint8* pixels = bitmap.GetPixels();
        const int stride = bitmap.Stride();
        const int bw = bitmap.Width();
        const int bh = bitmap.Height();
        if(!pixels || stride<=0 || bw<=0 || bh<=0)
        {
            return NULL;
        }

        // Single-slot GPU bitmap cache is only safe while the same PlatformBitmap
        // object stays alive. Heap address reuse of freed CPU buffers used to
        // make subsequent icons paint with the previous image.
        PlatformBitmap* pb = bitmap.platform_bitmap();
        if(cached_bitmap_ && cached_pb_==pb && pb!=NULL &&
            cached_bw_==bw && cached_bh_==bh && cached_stride_==stride &&
            cached_pixels_==pixels)
        {
            return cached_bitmap_;
        }

        InvalidateBitmapCache();
        const D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_PREMULTIPLIED));
        ID2D1Bitmap* d2d_bitmap = NULL;
        const HRESULT hr = rt_->CreateBitmap(
            D2D1::SizeU(static_cast<UINT>(bw), static_cast<UINT>(bh)),
            pixels,
            static_cast<UINT>(stride),
            props,
            &d2d_bitmap);
        if(FAILED(hr) || !d2d_bitmap)
        {
            return NULL;
        }
        cached_bitmap_ = d2d_bitmap;
        cached_pb_ = pb;
        cached_pixels_ = pixels;
        cached_bw_ = bw;
        cached_bh_ = bh;
        cached_stride_ = stride;
        return cached_bitmap_;
    }

    bool CanvasD2D::CopyToHdc(HDC hdc, int dest_x, int dest_y,
        int src_x, int src_y, int width, int height)
    {
        if(drawing_)
        {
            if(!EndDraw())
            {
                return false;
            }
        }
        if(!wic_bitmap_ || width<=0 || height<=0)
        {
            return false;
        }
        if(src_x<0 || src_y<0 || src_x+width>width_ || src_y+height>height_)
        {
            return false;
        }

        WICRect lock_rect = { src_x, src_y, width, height };
        IWICBitmapLock* lock = NULL;
        HRESULT hr = wic_bitmap_->Lock(&lock_rect, WICBitmapLockRead, &lock);
        if(FAILED(hr) || !lock)
        {
            return false;
        }

        UINT buf_size = 0;
        BYTE* data = NULL;
        hr = lock->GetDataPointer(&buf_size, &data);
        UINT stride = 0;
        if(SUCCEEDED(hr))
        {
            hr = lock->GetStride(&stride);
        }
        if(FAILED(hr) || !data || stride==0)
        {
            lock->Release();
            return false;
        }

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        const BYTE* packed = data;
        std::vector<BYTE> packed_storage;
        if(stride != static_cast<UINT>(width*4))
        {
            packed_storage.resize(static_cast<size_t>(width*4*height));
            for(int row=0; row<height; ++row)
            {
                memcpy(&packed_storage[static_cast<size_t>(row*width*4)],
                    data + static_cast<size_t>(row)*stride,
                    static_cast<size_t>(width*4));
            }
            packed = packed_storage.empty() ? NULL : &packed_storage[0];
        }
        if(!packed)
        {
            lock->Release();
            return false;
        }

        bool ok = false;
        if(hdc)
        {
            ok = SetDIBitsToDevice(hdc, dest_x, dest_y, width, height,
                0, 0, 0, height, packed, &bmi, DIB_RGB_COLORS) != 0;
        }
        else
        {
            platform_dc_ = CreateCompatibleDC(NULL);
            if(platform_dc_)
            {
                void* bits = NULL;
                platform_dib_ = CreateDIBSection(platform_dc_, &bmi, DIB_RGB_COLORS,
                    &bits, NULL, 0);
                if(platform_dib_ && bits)
                {
                    memcpy(bits, packed, static_cast<size_t>(width*4*height));
                    platform_old_ = SelectObject(platform_dc_, platform_dib_);
                    platform_bits_ = bits;
                    ok = true;
                }
            }
            if(!ok)
            {
                ReleasePlatformDc();
            }
        }

        lock->Release();
        return ok;
    }

    ID2D1SolidColorBrush* CanvasD2D::BrushFor(const Color& color)
    {
        if(!brush_)
        {
            return NULL;
        }
        brush_->SetColor(ToD2DColor(color));
        return brush_;
    }

    D2D1_COLOR_F CanvasD2D::ToD2DColor(const Color& color)
    {
        return D2D1::ColorF(
            color.GetR()/255.0f,
            color.GetG()/255.0f,
            color.GetB()/255.0f,
            color.GetA()/255.0f);
    }

    D2D1_RECT_F CanvasD2D::ToD2DRect(int x, int y, int w, int h)
    {
        return D2D1::RectF(
            static_cast<FLOAT>(x),
            static_cast<FLOAT>(y),
            static_cast<FLOAT>(x+w),
            static_cast<FLOAT>(y+h));
    }

    Canvas* Canvas::CreateCanvas()
    {
        return new CanvasD2D;
    }

    Canvas* Canvas::CreateCanvas(int width, int height, bool is_opaque)
    {
        return new CanvasD2D(width, height, is_opaque);
    }

    class CanvasPaintWin : public CanvasD2DPaint, public CanvasPaint
    {
    public:
        CanvasPaintWin(HWND view) : CanvasD2DPaint(view) {}

        virtual bool IsValid() const
        {
            return isEmpty();
        }

        virtual Rect GetInvalidRect() const
        {
            return Rect(paintStruct().rcPaint);
        }

        virtual Canvas* AsCanvas()
        {
            return this;
        }
    };

    CanvasPaint* CanvasPaint::CreateCanvasPaint(HWND view)
    {
        return new CanvasPaintWin(view);
    }

} //namespace gfx
