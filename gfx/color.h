
#ifndef __color_h__
#define __color_h__

#pragma once

#include <windows.h>

#include "base/basic_types.h"

namespace gfx
{

    class Color
    {
    public:
        Color() : argb_(0xFF000000) {}
        Color(COLORREF color) { SetFromCOLORREF(color); }
        Color(uint8 r, uint8 g, uint8 b) : argb_(MakeARGB(255, r, g, b)) {}
        Color(uint8 a, uint8 r, uint8 g, uint8 b)
            : argb_(MakeARGB(a, r, g, b)) {}

        uint8 GetA() const { return static_cast<uint8>((argb_>>24) & 0xFF); }
        uint8 GetR() const { return static_cast<uint8>((argb_>>16) & 0xFF); }
        uint8 GetG() const { return static_cast<uint8>((argb_>>8) & 0xFF); }
        uint8 GetB() const { return static_cast<uint8>(argb_ & 0xFF); }

        uint32 GetValue() const { return argb_; }
        void SetValue(uint32 argb) { argb_ = argb; }
        COLORREF ToCOLORREF() const
        {
            return RGB(GetR(), GetG(), GetB());
        }
        void SetFromCOLORREF(COLORREF rgb)
        {
            argb_ = MakeARGB(255,
                GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));
        }

        static uint32 MakeARGB(uint8 a, uint8 r, uint8 g, uint8 b)
        {
            return (static_cast<uint32>(a)<<24) |
                (static_cast<uint32>(r)<<16) |
                (static_cast<uint32>(g)<<8) |
                static_cast<uint32>(b);
        }

    private:
        uint32 argb_;
    };

    static const Color ColorWHITE(255, 255, 255);

} //namespace gfx

#endif //__color_h__
