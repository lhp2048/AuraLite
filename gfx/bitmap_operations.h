
#ifndef __bitmap_operations_h__
#define __bitmap_operations_h__

#pragma once

#include "bitmap.h"

namespace gfx
{

    class BitmapOperations
    {
    public:
        // Bright-red 32x32 placeholder used when an image fails to load.
        static Bitmap CreateDebugBitmap();

        // Blend two same-size 32bpp BGRA images. |alpha| is the second image.
        static Bitmap CreateBlendedBitmap(const Bitmap& first_bitmap,
            const Bitmap& second_bitmap, double alpha);

        // Paint |color| then |image|, then apply |mask|. 32bpp BGRA.
        static Bitmap CreateButtonBackground(const Color& color,
            const Bitmap& image_bitmap, const Bitmap& mask_bitmap);

    private:
        BitmapOperations();
    };

} //namespace gfx

#endif //__bitmap_operations_h__
