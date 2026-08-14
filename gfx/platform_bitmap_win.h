
#ifndef __platform_bitmap_win_h__
#define __platform_bitmap_win_h__

#pragma once

#include <vector>

#include "base/scoped_ptr.h"

#include "platform_bitmap.h"

namespace gfx
{

    class PlatformBitmapWin : public PlatformBitmap
    {
    public:
        explicit PlatformBitmapWin(Gdiplus::Bitmap* native_bitmap);
        PlatformBitmapWin(int width, int height, int stride,
            const std::vector<uint8>& pixels);

        virtual Gdiplus::Bitmap* GetNativeBitmap() const;

        virtual int Width() const;

        virtual int Height() const;

        virtual const uint8* GetPixels() const;

        virtual int Stride() const;

    private:
        virtual ~PlatformBitmapWin() {}

        class BitmapRef : public base::RefCounted<BitmapRef>
        {
        public:
            BitmapRef(Gdiplus::Bitmap* native_bitmap);
            BitmapRef(int width, int height, int stride,
                const std::vector<uint8>& pixels);

            Gdiplus::Bitmap* bitmap() const;
            int width() const { return width_; }
            int height() const { return height_; }
            int stride() const { return stride_; }
            const uint8* pixels() const
            {
                return pixels_.empty() ? NULL : &pixels_[0];
            }

        private:
            friend class base::RefCounted<BitmapRef>;

            void CopyPixelsFromGdiplus(Gdiplus::Bitmap* native_bitmap);

            int width_;
            int height_;
            int stride_;
            std::vector<uint8> pixels_;
            mutable scoped_ptr<Gdiplus::Bitmap> bitmap_;

            DISALLOW_COPY_AND_ASSIGN(BitmapRef);
        };

        scoped_refptr<BitmapRef> bitmap_ref_;
    };

} //namespace gfx

#endif //__platform_bitmap_win_h__
