
#ifndef __bitmap_h__
#define __bitmap_h__

#pragma once

#include <vector>
#include <windows.h>

#include "base/ref_counted.h"

#include "color.h"

namespace gfx
{

    class PlatformBitmap;

    class Bitmap
    {
    public:
        Bitmap();

        Bitmap(const Bitmap& other);
        Bitmap& operator=(const Bitmap& other);

        // 用PlatformBitmap对象构建Bitmap. Bitmap对象接管PlatformBitmap对象的所有权.
        explicit Bitmap(PlatformBitmap* platform_bitmap);

        ~Bitmap();

        PlatformBitmap* platform_bitmap() const { return platform_bitmap_.get(); }

        bool IsNull() const;

        int Width() const;
        int Height() const;

        Color GetPixel(int x, int y) const;

        // Decode PNG/JPEG/BMP/etc via WIC into a 32bpp premultiplied BGRA buffer.
        static Bitmap DecodeFromMemory(const void* data, size_t size);

        // Own a 32bpp premultiplied BGRA CPU buffer.
        static Bitmap CreateFromPixels(int width, int height, int stride,
            const std::vector<uint8>& pixels);

        // 32bpp premultiplied BGRA pixels, or NULL if the bitmap has no CPU buffer.
        const uint8* GetPixels() const;
        int Stride() const;

        // Caller owns the returned HICON (DestroyIcon). NULL if empty.
        HICON CreateHICON() const;

    private:
        scoped_refptr<PlatformBitmap> platform_bitmap_;
    };

} //namespace gfx

#endif //__bitmap_h__
