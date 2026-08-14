
#ifndef __platform_bitmap_win_h__
#define __platform_bitmap_win_h__

#pragma once

#include <vector>

#include "platform_bitmap.h"

namespace gfx
{

    class PlatformBitmapWin : public PlatformBitmap
    {
    public:
        PlatformBitmapWin(int width, int height, int stride,
            const std::vector<uint8>& pixels);

        virtual int Width() const;
        virtual int Height() const;
        virtual const uint8* GetPixels() const;
        virtual int Stride() const;

    private:
        virtual ~PlatformBitmapWin() {}

        class BitmapRef : public base::RefCounted<BitmapRef>
        {
        public:
            BitmapRef(int width, int height, int stride,
                const std::vector<uint8>& pixels);

            int width() const { return width_; }
            int height() const { return height_; }
            int stride() const { return stride_; }
            const uint8* pixels() const
            {
                return pixels_.empty() ? NULL : &pixels_[0];
            }

        private:
            friend class base::RefCounted<BitmapRef>;

            int width_;
            int height_;
            int stride_;
            std::vector<uint8> pixels_;

            DISALLOW_COPY_AND_ASSIGN(BitmapRef);
        };

        scoped_refptr<BitmapRef> bitmap_ref_;
    };

} //namespace gfx

#endif //__platform_bitmap_win_h__
