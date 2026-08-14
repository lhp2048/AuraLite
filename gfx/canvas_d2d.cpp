
#include "canvas_d2d.h"

#include <wincodec.h>

#include "base/logging.h"

#include "brush.h"
#include "color.h"
#include "font.h"
#include "rect.h"

#pragma comment(lib, "d2d1.lib")
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

}

namespace gfx
{

    CanvasD2D::CanvasD2D()
        : d2d_factory_(NULL),
          bitmap_rt_(NULL),
          brush_(NULL),
          width_(0),
          height_(0),
          is_opaque_(false),
          drawing_(false),
          clip_depth_(0) {}

    CanvasD2D::CanvasD2D(int width, int height, bool is_opaque)
        : d2d_factory_(NULL),
          bitmap_rt_(NULL),
          brush_(NULL),
          width_(0),
          height_(0),
          is_opaque_(false),
          drawing_(false),
          clip_depth_(0)
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

        IWICBitmap* wic_bitmap = NULL;
        hr = wic->CreateBitmap(static_cast<UINT>(width),
            static_cast<UINT>(height), GUID_WICPixelFormat32bppPBGRA,
            WICBitmapCacheOnLoad, &wic_bitmap);
        if(FAILED(hr) || !wic_bitmap)
        {
            wic->Release();
            return false;
        }

        const D2D1_PIXEL_FORMAT pixel_format = D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
        const D2D1_RENDER_TARGET_PROPERTIES rt_props =
            D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
                pixel_format);

        ID2D1RenderTarget* parent_rt = NULL;
        hr = d2d_factory_->CreateWicBitmapRenderTarget(wic_bitmap, rt_props,
            &parent_rt);
        wic_bitmap->Release();
        wic->Release();
        if(FAILED(hr) || !parent_rt)
        {
            return false;
        }

        hr = parent_rt->CreateCompatibleRenderTarget(
            D2D1::SizeF(static_cast<FLOAT>(width), static_cast<FLOAT>(height)),
            D2D1::SizeU(static_cast<UINT32>(width), static_cast<UINT32>(height)),
            pixel_format,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE,
            &bitmap_rt_);
        parent_rt->Release();
        if(FAILED(hr) || !bitmap_rt_)
        {
            bitmap_rt_ = NULL;
            return false;
        }

        hr = bitmap_rt_->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::Black), &brush_);
        if(FAILED(hr) || !brush_)
        {
            DiscardDeviceResources();
            return false;
        }

        return true;
    }

    bool CanvasD2D::BeginDraw()
    {
        if(!bitmap_rt_)
        {
            return false;
        }
        if(drawing_)
        {
            return true;
        }
        bitmap_rt_->BeginDraw();
        drawing_ = true;
        return true;
    }

    bool CanvasD2D::EndDraw()
    {
        if(!bitmap_rt_ || !drawing_)
        {
            return false;
        }
        const HRESULT hr = bitmap_rt_->EndDraw();
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
        if(!bitmap_rt_)
        {
            return;
        }
        EnsureDrawing();
        bitmap_rt_->Clear(ToD2DColor(color));
    }

    void CanvasD2D::Save()
    {
        if(!bitmap_rt_ || !d2d_factory_)
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
            bitmap_rt_->SaveDrawingState(state.block);
        }
        states_.push(state);
    }

    void CanvasD2D::Restore()
    {
        if(states_.empty() || !bitmap_rt_)
        {
            NOTREACHED();
            return;
        }

        const SavedState state = states_.top();
        states_.pop();
        while(clip_depth_>state.clip_depth)
        {
            bitmap_rt_->PopAxisAlignedClip();
            --clip_depth_;
        }
        if(state.block)
        {
            bitmap_rt_->RestoreDrawingState(state.block);
            state.block->Release();
        }
    }

    void CanvasD2D::SaveLayer(uint8 /*alpha*/)
    {
        NOTREACHED();
    }

    void CanvasD2D::SaveLayer(uint8 /*alpha*/, const Rect& /*layer_bounds*/)
    {
        NOTREACHED();
    }

    void CanvasD2D::RestoreLayer()
    {
        NOTREACHED();
    }

    bool CanvasD2D::ClipRectInt(int x, int y, int w, int h)
    {
        if(!bitmap_rt_)
        {
            return false;
        }
        EnsureDrawing();
        bitmap_rt_->PushAxisAlignedClip(
            ToD2DRect(x, y, w, h), D2D1_ANTIALIAS_MODE_ALIASED);
        ++clip_depth_;
        // true = non-empty clip, matching View::ProcessPaint.
        return w>0 && h>0;
    }

    void CanvasD2D::TranslateInt(int x, int y)
    {
        if(!bitmap_rt_)
        {
            return;
        }
        EnsureDrawing();
        D2D1_MATRIX_3X2_F current;
        bitmap_rt_->GetTransform(&current);
        bitmap_rt_->SetTransform(
            current * D2D1::Matrix3x2F::Translation(
                static_cast<FLOAT>(x), static_cast<FLOAT>(y)));
    }

    void CanvasD2D::ScaleInt(int x, int y)
    {
        if(!bitmap_rt_)
        {
            return;
        }
        EnsureDrawing();
        D2D1_MATRIX_3X2_F current;
        bitmap_rt_->GetTransform(&current);
        bitmap_rt_->SetTransform(
            current * D2D1::Matrix3x2F::Scale(
                static_cast<FLOAT>(x), static_cast<FLOAT>(y)));
    }

    void CanvasD2D::FillRectInt(const Color& color, int x, int y, int w, int h)
    {
        if(!bitmap_rt_ || w<=0 || h<=0)
        {
            return;
        }
        EnsureDrawing();
        ID2D1SolidColorBrush* brush = BrushFor(color);
        if(!brush)
        {
            return;
        }
        bitmap_rt_->FillRectangle(ToD2DRect(x, y, w, h), brush);
    }

    void CanvasD2D::FillRectInt(const Brush& /*brush*/,
        int /*x*/, int /*y*/, int /*w*/, int /*h*/)
    {
        NOTREACHED();
    }

    void CanvasD2D::DrawRectInt(const Color& color, int x, int y, int w, int h)
    {
        if(!bitmap_rt_ || w<=0 || h<=0)
        {
            return;
        }
        EnsureDrawing();
        ID2D1SolidColorBrush* brush = BrushFor(color);
        if(!brush)
        {
            return;
        }
        bitmap_rt_->DrawRectangle(ToD2DRect(x, y, w, h), brush, 1.0f);
    }

    void CanvasD2D::DrawLineInt(const Color& /*color*/,
        int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/)
    {
        NOTREACHED();
    }

    void CanvasD2D::DrawBitmapInt(const Bitmap& /*bitmap*/, int /*x*/, int /*y*/)
    {
        NOTREACHED();
    }

    void CanvasD2D::DrawBitmapInt(const Bitmap& /*bitmap*/,
        int /*src_x*/, int /*src_y*/, int /*src_w*/, int /*src_h*/,
        int /*dest_x*/, int /*dest_y*/, int /*dest_w*/, int /*dest_h*/)
    {
        NOTREACHED();
    }

    void CanvasD2D::DrawStringInt(const std::wstring& /*text*/,
        const Font& /*font*/,
        const Color& /*color*/,
        int /*x*/, int /*y*/, int /*w*/, int /*h*/)
    {
        NOTREACHED();
    }

    void CanvasD2D::DrawStringInt(const std::wstring& /*text*/,
        const Font& /*font*/,
        const Color& /*color*/,
        const Rect& /*display_rect*/)
    {
        NOTREACHED();
    }

    void CanvasD2D::DrawStringInt(const std::wstring& /*text*/,
        const Font& /*font*/,
        const Color& /*color*/,
        int /*x*/, int /*y*/, int /*w*/, int /*h*/,
        int /*flags*/)
    {
        NOTREACHED();
    }

    void CanvasD2D::DrawFocusRect(int /*x*/, int /*y*/,
        int /*width*/, int /*height*/)
    {
        NOTREACHED();
    }

    void CanvasD2D::TileImageInt(const Bitmap& /*bitmap*/,
        int /*x*/, int /*y*/, int /*w*/, int /*h*/)
    {
        NOTREACHED();
    }

    void CanvasD2D::TileImageInt(const Bitmap& /*bitmap*/,
        int /*src_x*/, int /*src_y*/,
        int /*dest_x*/, int /*dest_y*/, int /*w*/, int /*h*/)
    {
        NOTREACHED();
    }

    HDC CanvasD2D::BeginPlatformPaint()
    {
        NOTREACHED();
        return NULL;
    }

    void CanvasD2D::EndPlatformPaint(HDC /*dc*/)
    {
        NOTREACHED();
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
        if(bitmap_rt_ && drawing_)
        {
            while(clip_depth_>0)
            {
                bitmap_rt_->PopAxisAlignedClip();
                --clip_depth_;
            }
            bitmap_rt_->EndDraw();
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
    }

    void CanvasD2D::DiscardDeviceResources()
    {
        SafeRelease(brush_);
        SafeRelease(bitmap_rt_);
    }

    void CanvasD2D::DiscardResources()
    {
        FlushDrawState();
        DiscardDeviceResources();
        SafeRelease(d2d_factory_);
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

} //namespace gfx
