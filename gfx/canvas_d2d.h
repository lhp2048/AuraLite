
#ifndef __canvas_d2d_h__
#define __canvas_d2d_h__

#pragma once

#include <windows.h>
#include <d2d1.h>

#include <stack>

#include "base/basic_types.h"

#include "canvas.h"
#include "canvas_paint_win.h"
#include "rect.h"

struct IWICBitmap;

namespace gfx
{

    class PlatformBitmap;

    // gfx::Canvas Direct2D offscreen backend (WIC bitmap + ID2D1RenderTarget).
    class CanvasD2D : public Canvas
    {
    public:
        CanvasD2D();
        CanvasD2D(int width, int height, bool is_opaque);
        virtual ~CanvasD2D();

        // Second-step init after the default constructor.
        bool initialize(int width, int height, bool is_opaque);

        bool BeginDraw();
        bool EndDraw();

        virtual void Clear(const Color& color);

        // Blit the offscreen bitmap to |hdc|. Ends an open BeginDraw first.
        void DrawToHDC(HDC hdc, int x, int y, const RECT* src_rect);

        ID2D1RenderTarget* render_target() const { return rt_; }

        virtual void Save();
        virtual void Restore();
        virtual void SaveLayer(uint8 alpha);
        virtual void SaveLayer(uint8 alpha, const Rect& layer_bounds);
        virtual void RestoreLayer();
        virtual bool ClipRectInt(int x, int y, int w, int h);
        virtual void TranslateInt(int x, int y);
        virtual void ScaleInt(int x, int y);
        virtual void FillRectInt(const Color& color, int x, int y, int w, int h);
        virtual void FillRectInt(const Brush& brush, int x, int y, int w, int h);
        virtual void DrawRectInt(const Color& color, int x, int y, int w, int h);
        virtual void FillEllipseInt(const Color& color, int x, int y, int w, int h);
        virtual void DrawEllipseInt(const Color& color, int x, int y, int w, int h);
        virtual void FillRoundedRectInt(const Color& color,
            int x, int y, int w, int h, int radius);
        virtual void DrawLineInt(const Color& color, int x1, int y1, int x2, int y2);
        virtual void DrawBitmapInt(const Bitmap& bitmap, int x, int y);
        virtual void DrawBitmapInt(const Bitmap& bitmap,
            int src_x, int src_y, int src_w, int src_h,
            int dest_x, int dest_y, int dest_w, int dest_h);
        virtual void DrawStringInt(const std::wstring& text,
            const Font& font,
            const Color& color,
            int x, int y, int w, int h);
        virtual void DrawStringInt(const std::wstring& text,
            const Font& font,
            const Color& color,
            const Rect& display_rect);
        virtual void DrawStringInt(const std::wstring& text,
            const Font& font,
            const Color& color,
            int x, int y, int w, int h,
            int flags);
        virtual void DrawFocusRect(int x, int y, int width, int height);
        virtual void TileImageInt(const Bitmap& bitmap,
            int x, int y, int w, int h);
        virtual void TileImageInt(const Bitmap& bitmap,
            int src_x, int src_y,
            int dest_x, int dest_y, int w, int h);
        virtual HDC BeginPlatformPaint();
        virtual void EndPlatformPaint(HDC dc);
        virtual CanvasD2D* AsCanvasD2D();
        virtual const CanvasD2D* AsCanvasD2D() const;

    private:
        struct SavedState
        {
            ID2D1DrawingStateBlock* block;
            int clip_depth;
        };

        void EnsureDrawing();
        void FlushDrawState();
        void DiscardDeviceResources();
        void DiscardResources();
        void ReleasePlatformDc();
        void PushOpacityLayer(uint8 alpha, const D2D1_RECT_F* content_bounds);
        void PopRemainingLayers();
        bool CopyToHdc(HDC hdc, int dest_x, int dest_y,
            int src_x, int src_y, int width, int height);
        bool CopyPlatformDcToBitmap();
        bool RecreateRenderTargetFromBitmap();
        void InvalidateBitmapCache();
        ID2D1Bitmap* GetOrCreateD2DBitmap(const Bitmap& bitmap);
        ID2D1SolidColorBrush* BrushFor(const Color& color);
        static D2D1_COLOR_F ToD2DColor(const Color& color);
        static D2D1_RECT_F ToD2DRect(int x, int y, int w, int h);

        ID2D1Factory* d2d_factory_;
        ID2D1RenderTarget* rt_;
        IWICBitmap* wic_bitmap_;
        ID2D1SolidColorBrush* brush_;
        ID2D1Bitmap* cached_bitmap_;
        PlatformBitmap* cached_pb_;
        const uint8* cached_pixels_;
        int cached_bw_;
        int cached_bh_;
        int cached_stride_;
        ID2D1Layer* recycled_layer_;
        HDC platform_dc_;
        HBITMAP platform_dib_;
        HGDIOBJ platform_old_;
        void* platform_bits_;
        int width_;
        int height_;
        bool is_opaque_;
        bool drawing_;
        int clip_depth_;
        int layer_depth_;
        std::stack<SavedState> states_;
        std::stack<ID2D1Layer*> layers_;

        DISALLOW_COPY_AND_ASSIGN(CanvasD2D);
    };

    typedef CanvasPaintT<CanvasD2D> CanvasD2DPaint;

} //namespace gfx

#endif //__canvas_d2d_h__
