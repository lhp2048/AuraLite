
#ifndef __platform_bitmap_h__
#define __platform_bitmap_h__

#pragma once

#include <vector>

#include "base/basic_types.h"
#include "base/ref_counted.h"

namespace gfx
{

    class PlatformBitmap : public base::RefCounted<PlatformBitmap>
    {
    public:
        // Decode PNG/JPEG/BMP/etc via WIC into a 32bpp premultiplied BGRA buffer.
        static PlatformBitmap* CreateFromEncodedMemory(
            const void* data, size_t size);

        // Own a 32bpp premultiplied BGRA CPU buffer (4 bytes per pixel).
        static PlatformBitmap* CreateFromPixels(int width, int height,
            int stride, const std::vector<uint8>& pixels);

        virtual int Width() const = 0;
        virtual int Height() const = 0;

        // 32bpp premultiplied BGRA, or NULL.
        virtual const uint8* GetPixels() const = 0;
        virtual int Stride() const = 0;

    protected:
        virtual ~PlatformBitmap() {}

    private:
        friend class base::RefCounted<PlatformBitmap>;
    };

} //namespace gfx

#endif //__platform_bitmap_h__
