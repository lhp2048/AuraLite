#include <windows.h>
#include <objbase.h>

#include <iostream>
#include <string>

#include "base/basic_types.h"
#include "gfx/canvas.h"
#include "gfx/canvas_d2d.h"
#include "gfx/color.h"
#include "gfx/dwrite_text.h"
#include "gfx/font.h"

namespace
{

int Fail(const char* msg)
{
    std::cerr << "FAIL: " << msg << std::endl;
    return 1;
}

} // namespace

int main()
{
    const HRESULT co = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if(FAILED(co) && co!=S_FALSE && co!=RPC_E_CHANGED_MODE)
    {
        return Fail("CoInitializeEx");
    }

    if(!gfx::dwrite_text::GetFactory())
    {
        CoUninitialize();
        return Fail("IDWriteFactory");
    }

    const gfx::Font font(L"Segoe UI", 12);

    int empty_w = -1;
    int empty_h = -1;
    gfx::dwrite_text::MeasureString(L"", font, gfx::Canvas::NO_ELLIPSIS,
        &empty_w, &empty_h);
    if(empty_w!=0)
    {
        CoUninitialize();
        return Fail("empty string width");
    }

    int hello_w = 0;
    int hello_h = 0;
    gfx::dwrite_text::MeasureString(L"Hello", font, gfx::Canvas::NO_ELLIPSIS,
        &hello_w, &hello_h);
    if(hello_w<=0 || hello_h<=0)
    {
        CoUninitialize();
        return Fail("Hello metrics");
    }

    int world_w = 0;
    int world_h = 0;
    gfx::dwrite_text::MeasureString(L"Hello World", font,
        gfx::Canvas::NO_ELLIPSIS, &world_w, &world_h);
    if(world_w<=hello_w)
    {
        CoUninitialize();
        return Fail("longer string should be wider");
    }

    const int via_font = font.GetStringWidth(L"Hello");
    if(via_font<=0)
    {
        CoUninitialize();
        return Fail("Font::GetStringWidth");
    }

    gfx::CanvasD2D canvas(320, 64, true);
    if(!canvas.render_target())
    {
        CoUninitialize();
        return Fail("CanvasD2D initialize");
    }

    canvas.Clear(gfx::Color(255, 255, 255));
    const int flags = gfx::Canvas::TEXT_ALIGN_LEFT |
        gfx::Canvas::TEXT_VALIGN_MIDDLE |
        gfx::Canvas::NO_ELLIPSIS;
    canvas.DrawStringInt(L"Hello DirectWrite", font, gfx::Color(0, 0, 0),
        8, 8, 300, 48, flags);
    canvas.DrawStringInt(L"Center", font, gfx::Color(0, 0, 0),
        8, 8, 300, 48,
        gfx::Canvas::TEXT_ALIGN_CENTER | gfx::Canvas::TEXT_VALIGN_TOP |
        gfx::Canvas::NO_ELLIPSIS);
    canvas.DrawStringInt(L"wrap line one wrap line two wrap", font,
        gfx::Color(0, 0, 0), 8, 8, 80, 56,
        gfx::Canvas::TEXT_ALIGN_LEFT | gfx::Canvas::TEXT_VALIGN_TOP |
        gfx::Canvas::MULTI_LINE | gfx::Canvas::NO_ELLIPSIS);
    if(!canvas.EndDraw())
    {
        CoUninitialize();
        return Fail("EndDraw");
    }

    std::cout << "OK hello_w=" << hello_w << " font_w=" << via_font
        << " world_w=" << world_w << std::endl;
    CoUninitialize();
    return 0;
}
