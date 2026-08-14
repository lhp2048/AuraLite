
#ifndef __dwrite_text_h__
#define __dwrite_text_h__

#pragma once

#include <dwrite.h>

#include <string>

namespace gfx
{

    class Font;

    // Shared DirectWrite helpers for CanvasD2D drawing and PlatformFontWin metrics.
    namespace dwrite_text
    {

        IDWriteFactory* GetFactory();

        // Caller must Release() a non-NULL layout.
        IDWriteTextLayout* CreateLayout(const std::wstring& text,
            const Font& font,
            int flags,
            float max_width,
            float max_height);

        void MeasureString(const std::wstring& text,
            const Font& font,
            int flags,
            int* width,
            int* height);

    } //namespace dwrite_text

} //namespace gfx

#endif //__dwrite_text_h__
