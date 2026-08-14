
#include "bitmap.h"

#include "platform_bitmap.h"

namespace gfx
{

    Bitmap::Bitmap() {}

    Bitmap::Bitmap(const Bitmap& other) : platform_bitmap_(
        other.platform_bitmap_) {}

    Bitmap& Bitmap::operator=(const Bitmap& other)
    {
        platform_bitmap_ = other.platform_bitmap_;
        return *this;
    }

    Bitmap::Bitmap(Gdiplus::Bitmap* native_bitmap) : platform_bitmap_(
        PlatformBitmap::CreateFromNativeBitmap(native_bitmap)) {}

    Bitmap::Bitmap(PlatformBitmap* platform_bitmap)
        : platform_bitmap_(platform_bitmap) {}

    Bitmap::~Bitmap() {}

    Gdiplus::Bitmap* Bitmap::GetNativeBitmap() const
    {
        if(IsNull())
        {
            return NULL;
        }

        return platform_bitmap_->GetNativeBitmap();
    }

    bool Bitmap::IsNull() const
    {
        return !platform_bitmap_.get();
    }

    int Bitmap::Width() const
    {
        if(!IsNull())
        {
            return platform_bitmap_->Width();
        }

        return 0;
    }

    int Bitmap::Height() const
    {
        if(!IsNull())
        {
            return platform_bitmap_->Height();
        }

        return 0;
    }

    Color Bitmap::GetPixel(int x, int y) const
    {
        Color color;
        if(IsNull() || x<0 || y<0 || x>=Width() || y>=Height())
        {
            return color;
        }

        const uint8* pixels = GetPixels();
        const int stride = Stride();
        if(pixels && stride>=4)
        {
            const uint8* px = pixels + y*stride + x*4;
            const uint32 value = (static_cast<uint32>(px[3])<<24) |
                (static_cast<uint32>(px[2])<<16) |
                (static_cast<uint32>(px[1])<<8) |
                static_cast<uint32>(px[0]);
            color.SetValue(value);
            return color;
        }

        Gdiplus::Bitmap* native = platform_bitmap_->GetNativeBitmap();
        if(!native)
        {
            return color;
        }
        Gdiplus::Color native_color;
        native->GetPixel(x, y, &native_color);
        color.SetValue(native_color.GetValue());
        return color;
    }

    Bitmap Bitmap::DecodeFromMemory(const void* data, size_t size)
    {
        PlatformBitmap* platform =
            PlatformBitmap::CreateFromEncodedMemory(data, size);
        if(!platform)
        {
            return Bitmap();
        }
        return Bitmap(platform);
    }

    const uint8* Bitmap::GetPixels() const
    {
        if(IsNull())
        {
            return NULL;
        }
        return platform_bitmap_->GetPixels();
    }

    int Bitmap::Stride() const
    {
        if(IsNull())
        {
            return 0;
        }
        return platform_bitmap_->Stride();
    }

} //namespace gfx
