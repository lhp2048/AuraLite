#include <windows.h>
#include <objbase.h>

#include <iostream>

#include "gfx/bitmap.h"
#include "gfx/canvas_d2d.h"
#include "gfx/color.h"

namespace
{

int Fail(const char* msg)
{
    std::cerr << "FAIL: " << msg << std::endl;
    return 1;
}

// 2x2 32bpp BI_RGB BMP, bottom-up. Top row: red, green. Bottom row: blue, white.
const unsigned char kTwoByTwoBmp[] =
{
    0x42, 0x4D, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00,
    0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // row y=1 (bottom in file): blue, white
    0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    // row y=0 (top in file): red, green
    0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0xFF
};

bool PixelIs(const uint8* px, uint8 b, uint8 g, uint8 r, uint8 a)
{
    return px[0]==b && px[1]==g && px[2]==r && px[3]==a;
}

} // namespace

int main()
{
    const HRESULT co = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if(FAILED(co) && co!=S_FALSE && co!=RPC_E_CHANGED_MODE)
    {
        return Fail("CoInitializeEx");
    }

    int rc = 0;
    {
        const gfx::Bitmap bmp = gfx::Bitmap::DecodeFromMemory(
            kTwoByTwoBmp, sizeof(kTwoByTwoBmp));
        if(bmp.IsNull())
        {
            rc = Fail("DecodeFromMemory");
        }
        else if(bmp.Width()!=2 || bmp.Height()!=2)
        {
            rc = Fail("size");
        }
        else
        {
            const uint8* pixels = bmp.GetPixels();
            const int stride = bmp.Stride();
            if(!pixels || stride<8)
            {
                rc = Fail("GetPixels");
            }
            else if(!PixelIs(pixels + 0, 0x00, 0x00, 0xFF, 0xFF) ||
                !PixelIs(pixels + 4, 0x00, 0xFF, 0x00, 0xFF) ||
                !PixelIs(pixels + stride, 0xFF, 0x00, 0x00, 0xFF) ||
                !PixelIs(pixels + stride + 4, 0xFF, 0xFF, 0xFF, 0xFF))
            {
                rc = Fail("pixel colors");
            }
            else
            {
                gfx::CanvasD2D canvas(8, 8, false);
                if(!canvas.render_target())
                {
                    rc = Fail("CanvasD2D initialize");
                }
                else
                {
                    canvas.Clear(gfx::Color(0, 0, 0, 0));
                    canvas.DrawBitmapInt(bmp, 1, 2);
                    if(!canvas.EndDraw())
                    {
                        rc = Fail("EndDraw");
                    }
                    else
                    {
                        std::cout << "OK wic 2x2 DrawBitmapInt" << std::endl;
                    }
                }
            }
        }
    }

    CoUninitialize();
    return rc;
}
