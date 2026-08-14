
#include "platform_bitmap_win.h"

#include <windows.h>
#include <objbase.h>
#include <wincodec.h>

#include <cstring>

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

    IWICImagingFactory* CreateWicFactory()
    {
        IWICImagingFactory* wic = NULL;
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
        if(hr==CO_E_NOTINITIALIZED)
        {
            CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            hr = CoCreateInstance(CLSID_WICImagingFactory, NULL,
                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
        }
        if(FAILED(hr))
        {
            return NULL;
        }
        return wic;
    }

    bool DecodeWicToBgra(const void* data, size_t size,
        int* width, int* height, int* stride, std::vector<uint8>* pixels)
    {
        if(!data || size==0 || size>static_cast<size_t>(0xFFFFFFFF) ||
            !width || !height || !stride || !pixels)
        {
            return false;
        }

        IWICImagingFactory* wic = CreateWicFactory();
        if(!wic)
        {
            return false;
        }

        IWICStream* stream = NULL;
        HRESULT hr = wic->CreateStream(&stream);
        if(FAILED(hr) || !stream)
        {
            SafeRelease(wic);
            return false;
        }

        hr = stream->InitializeFromMemory(
            static_cast<BYTE*>(const_cast<void*>(data)),
            static_cast<DWORD>(size));
        if(FAILED(hr))
        {
            SafeRelease(stream);
            SafeRelease(wic);
            return false;
        }

        IWICBitmapDecoder* decoder = NULL;
        hr = wic->CreateDecoderFromStream(stream, NULL,
            WICDecodeMetadataCacheOnLoad, &decoder);
        if(FAILED(hr) || !decoder)
        {
            SafeRelease(stream);
            SafeRelease(wic);
            return false;
        }

        IWICBitmapFrameDecode* frame = NULL;
        hr = decoder->GetFrame(0, &frame);
        if(FAILED(hr) || !frame)
        {
            SafeRelease(decoder);
            SafeRelease(stream);
            SafeRelease(wic);
            return false;
        }

        IWICFormatConverter* converter = NULL;
        hr = wic->CreateFormatConverter(&converter);
        if(FAILED(hr) || !converter)
        {
            SafeRelease(frame);
            SafeRelease(decoder);
            SafeRelease(stream);
            SafeRelease(wic);
            return false;
        }

        hr = converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);
        if(FAILED(hr))
        {
            SafeRelease(converter);
            SafeRelease(frame);
            SafeRelease(decoder);
            SafeRelease(stream);
            SafeRelease(wic);
            return false;
        }

        UINT w = 0;
        UINT h = 0;
        hr = converter->GetSize(&w, &h);
        if(FAILED(hr) || w==0 || h==0)
        {
            SafeRelease(converter);
            SafeRelease(frame);
            SafeRelease(decoder);
            SafeRelease(stream);
            SafeRelease(wic);
            return false;
        }

        const UINT row_stride = w * 4;
        std::vector<uint8> buffer;
        buffer.resize(static_cast<size_t>(row_stride) * h);
        hr = converter->CopyPixels(NULL, row_stride,
            static_cast<UINT>(buffer.size()), &buffer[0]);

        SafeRelease(converter);
        SafeRelease(frame);
        SafeRelease(decoder);
        SafeRelease(stream);
        SafeRelease(wic);

        if(FAILED(hr))
        {
            return false;
        }

        *width = static_cast<int>(w);
        *height = static_cast<int>(h);
        *stride = static_cast<int>(row_stride);
        pixels->swap(buffer);
        return true;
    }

}

namespace gfx
{

    PlatformBitmapWin::PlatformBitmapWin(int width, int height, int stride,
        const std::vector<uint8>& pixels)
        : bitmap_ref_(new BitmapRef(width, height, stride, pixels)) {}

    int PlatformBitmapWin::Width() const
    {
        return bitmap_ref_->width();
    }

    int PlatformBitmapWin::Height() const
    {
        return bitmap_ref_->height();
    }

    const uint8* PlatformBitmapWin::GetPixels() const
    {
        return bitmap_ref_->pixels();
    }

    int PlatformBitmapWin::Stride() const
    {
        return bitmap_ref_->stride();
    }

    PlatformBitmapWin::BitmapRef::BitmapRef(int width, int height, int stride,
        const std::vector<uint8>& pixels)
        : width_(width),
          height_(height),
          stride_(stride),
          pixels_(pixels) {}

    // static
    PlatformBitmap* PlatformBitmap::CreateFromPixels(int width, int height,
        int stride, const std::vector<uint8>& pixels)
    {
        if(width<=0 || height<=0 || stride<width*4)
        {
            return NULL;
        }
        const size_t needed = static_cast<size_t>(stride) *
            static_cast<size_t>(height);
        if(pixels.size()<needed)
        {
            return NULL;
        }
        return new PlatformBitmapWin(width, height, stride, pixels);
    }

    // static
    PlatformBitmap* PlatformBitmap::CreateFromEncodedMemory(
        const void* data, size_t size)
    {
        int width = 0;
        int height = 0;
        int stride = 0;
        std::vector<uint8> pixels;
        if(!DecodeWicToBgra(data, size, &width, &height, &stride, &pixels))
        {
            return NULL;
        }
        return CreateFromPixels(width, height, stride, pixels);
    }

} //namespace gfx
