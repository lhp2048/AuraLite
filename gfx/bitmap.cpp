
#include "bitmap.h"

#include <cstring>

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

    Bitmap::Bitmap(PlatformBitmap* platform_bitmap)
        : platform_bitmap_(platform_bitmap) {}

    Bitmap::~Bitmap() {}

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
        if(!pixels || stride<4)
        {
            return color;
        }

        const uint8* px = pixels + y*stride + x*4;
        color.SetValue(Color::MakeARGB(px[3], px[2], px[1], px[0]));
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

    Bitmap Bitmap::CreateFromPixels(int width, int height, int stride,
        const std::vector<uint8>& pixels)
    {
        PlatformBitmap* platform = PlatformBitmap::CreateFromPixels(
            width, height, stride, pixels);
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

    HICON Bitmap::CreateHICON() const
    {
        const int width = Width();
        const int height = Height();
        const int stride = Stride();
        const uint8* src = GetPixels();
        if(width<=0 || height<=0 || stride<width*4 || !src)
        {
            return NULL;
        }

        BITMAPV5HEADER bi;
        memset(&bi, 0, sizeof(bi));
        bi.bV5Size = sizeof(BITMAPV5HEADER);
        bi.bV5Width = width;
        bi.bV5Height = -height;
        bi.bV5Planes = 1;
        bi.bV5BitCount = 32;
        bi.bV5Compression = BI_BITFIELDS;
        bi.bV5RedMask = 0x00FF0000;
        bi.bV5GreenMask = 0x0000FF00;
        bi.bV5BlueMask = 0x000000FF;
        bi.bV5AlphaMask = 0xFF000000;

        void* bits = NULL;
        HDC hdc = GetDC(NULL);
        HBITMAP color_bmp = CreateDIBSection(hdc,
            reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS, &bits, NULL, 0);
        ReleaseDC(NULL, hdc);
        if(!color_bmp || !bits)
        {
            if(color_bmp)
            {
                DeleteObject(color_bmp);
            }
            return NULL;
        }

        const int dest_stride = width * 4;
        uint8* dest = static_cast<uint8*>(bits);
        for(int y=0; y<height; ++y)
        {
            memcpy(dest + static_cast<size_t>(y)*dest_stride,
                src + static_cast<size_t>(y)*stride,
                static_cast<size_t>(dest_stride));
        }

        ICONINFO info;
        memset(&info, 0, sizeof(info));
        info.fIcon = TRUE;
        info.hbmColor = color_bmp;
        info.hbmMask = CreateBitmap(width, height, 1, 1, NULL);
        HICON icon = CreateIconIndirect(&info);
        DeleteObject(color_bmp);
        if(info.hbmMask)
        {
            DeleteObject(info.hbmMask);
        }
        return icon;
    }

} //namespace gfx
