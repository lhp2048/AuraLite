
#ifndef __platform_bitmap_h__
#define __platform_bitmap_h__

#pragma once

#include "base/basic_types.h"
#include "base/ref_counted.h"

namespace Gdiplus
{
    class Bitmap;
}

namespace gfx
{

    class Bitmap;

    class PlatformBitmap : public base::RefCounted<PlatformBitmap>
    {
    public:
        // 创建平台相关的PlatformBitmap.
        static PlatformBitmap* CreateFromNativeBitmap(
            Gdiplus::Bitmap* native_bitmap);

        // Decode PNG/JPEG/BMP/etc via WIC into a 32bpp premultiplied BGRA buffer.
        static PlatformBitmap* CreateFromEncodedMemory(
            const void* data, size_t size);

        // 返回本地位图对象.
        virtual Gdiplus::Bitmap* GetNativeBitmap() const = 0;

        // 获取位图的宽度.
        virtual int Width() const = 0;

        // 获取位图的高度.
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
