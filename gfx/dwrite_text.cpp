
#include "dwrite_text.h"

#include <cmath>
#include <windows.h>

#include "base/basic_types.h"
#include "base/rtl.h"

#include "canvas.h"
#include "font.h"

#pragma comment(lib, "dwrite.lib")

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

    const float kUnconstrainedLayout = 100000.0f;

    float FontSizeDip(const gfx::Font& font)
    {
        LOGFONT lf = { 0 };
        GetObject(font.GetNativeFont(), sizeof(lf), &lf);
        if(lf.lfHeight<0)
        {
            return static_cast<float>(-lf.lfHeight);
        }
        if(lf.lfHeight>0)
        {
            return static_cast<float>(lf.lfHeight);
        }
        return 12.0f;
    }

    void PreparePrefix(const std::wstring& in,
        int flags,
        std::wstring* out,
        UINT32* underline_pos,
        bool* has_underline)
    {
        *has_underline = false;
        *underline_pos = 0;
        if(!(flags&(gfx::Canvas::SHOW_PREFIX|gfx::Canvas::HIDE_PREFIX)))
        {
            *out = in;
            return;
        }

        const bool show = (flags&gfx::Canvas::SHOW_PREFIX)!=0;
        out->clear();
        out->reserve(in.size());
        for(size_t i=0; i<in.size(); ++i)
        {
            if(in[i]==L'&')
            {
                if(i+1<in.size() && in[i+1]==L'&')
                {
                    out->push_back(L'&');
                    ++i;
                    continue;
                }
                if(show && i+1<in.size() && !*has_underline)
                {
                    *underline_pos = static_cast<UINT32>(out->size());
                    *has_underline = true;
                }
                continue;
            }
            out->push_back(in[i]);
        }
    }

    IDWriteTextFormat* CreateFormat(IDWriteFactory* factory,
        const gfx::Font& font,
        int flags)
    {
        const int style = font.GetStyle();
        const DWRITE_FONT_WEIGHT weight = (style&gfx::Font::BOLD) ?
            DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
        const DWRITE_FONT_STYLE font_style = (style&gfx::Font::ITALIC) ?
            DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;

        wchar_t locale_buf[LOCALE_NAME_MAX_LENGTH] = { 0 };
        const wchar_t* locale = locale_buf;
        if(!GetUserDefaultLocaleName(locale_buf, LOCALE_NAME_MAX_LENGTH))
        {
            locale = L"en-US";
        }

        std::wstring family = font.GetFontName();
        if(family.empty())
        {
            family = L"Segoe UI";
        }

        IDWriteTextFormat* format = NULL;
        const HRESULT hr = factory->CreateTextFormat(
            family.c_str(),
            NULL,
            weight,
            font_style,
            DWRITE_FONT_STRETCH_NORMAL,
            FontSizeDip(font),
            locale,
            &format);
        if(FAILED(hr) || !format)
        {
            return NULL;
        }

        int align_flags = flags;
        if(!(align_flags&(gfx::Canvas::TEXT_ALIGN_LEFT|
            gfx::Canvas::TEXT_ALIGN_CENTER|gfx::Canvas::TEXT_ALIGN_RIGHT)))
        {
            align_flags |= base::IsRTL() ?
                gfx::Canvas::TEXT_ALIGN_RIGHT : gfx::Canvas::TEXT_ALIGN_LEFT;
        }

        if(align_flags&gfx::Canvas::TEXT_ALIGN_CENTER)
        {
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        else if(align_flags&gfx::Canvas::TEXT_ALIGN_RIGHT)
        {
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        }
        else
        {
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        }

        if(flags&gfx::Canvas::TEXT_VALIGN_TOP)
        {
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        }
        else if(flags&gfx::Canvas::TEXT_VALIGN_BOTTOM)
        {
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
        }
        else
        {
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        if(flags&gfx::Canvas::MULTI_LINE)
        {
            // CHARACTER_BREAK: allow wrapping mid-word / CJK grapheme (Win8.1+).
            if(flags&gfx::Canvas::CHARACTER_BREAK)
            {
                format->SetWordWrapping(DWRITE_WORD_WRAPPING_CHARACTER);
            }
            else
            {
                format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
            }
        }
        else
        {
            format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }

        if(flags&gfx::Canvas::FORCE_RTL_DIRECTIONALITY)
        {
            format->SetReadingDirection(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
        }

        return format;
    }

    void ApplyTrimming(IDWriteFactory* factory,
        IDWriteTextFormat* format,
        IDWriteTextLayout* layout,
        int flags)
    {
        if(flags&gfx::Canvas::NO_ELLIPSIS)
        {
            return;
        }

        IDWriteInlineObject* ellipsis = NULL;
        const HRESULT hr = factory->CreateEllipsisTrimmingSign(format, &ellipsis);
        if(FAILED(hr) || !ellipsis)
        {
            return;
        }

        DWRITE_TRIMMING trimming = {};
        trimming.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
        layout->SetTrimming(&trimming, ellipsis);
        ellipsis->Release();
    }

} // namespace

namespace gfx
{

    namespace dwrite_text
    {

        IDWriteFactory* GetFactory()
        {
            static IDWriteFactory* factory = NULL;
            if(!factory)
            {
                const HRESULT hr = DWriteCreateFactory(
                    DWRITE_FACTORY_TYPE_SHARED,
                    __uuidof(IDWriteFactory),
                    reinterpret_cast<IUnknown**>(&factory));
                if(FAILED(hr))
                {
                    factory = NULL;
                }
            }
            return factory;
        }

        IDWriteTextLayout* CreateLayout(const std::wstring& text,
            const Font& font,
            int flags,
            float max_width,
            float max_height)
        {
            IDWriteFactory* factory = GetFactory();
            if(!factory)
            {
                return NULL;
            }

            IDWriteTextFormat* format = CreateFormat(factory, font, flags);
            if(!format)
            {
                return NULL;
            }

            std::wstring display;
            UINT32 underline_pos = 0;
            bool has_mnemonic = false;
            PreparePrefix(text, flags, &display, &underline_pos, &has_mnemonic);

            if(max_width<=0.0f)
            {
                max_width = kUnconstrainedLayout;
            }
            if(max_height<=0.0f)
            {
                max_height = kUnconstrainedLayout;
            }

            IDWriteTextLayout* layout = NULL;
            const HRESULT hr = factory->CreateTextLayout(
                display.c_str(),
                static_cast<UINT32>(display.size()),
                format,
                max_width,
                max_height,
                &layout);
            if(FAILED(hr) || !layout)
            {
                format->Release();
                return NULL;
            }

            ApplyTrimming(factory, format, layout, flags);

            if((font.GetStyle()&Font::UNDERLINED) && !display.empty())
            {
                DWRITE_TEXT_RANGE range = { 0, static_cast<UINT32>(display.size()) };
                layout->SetUnderline(TRUE, range);
            }
            else if(has_mnemonic && underline_pos<display.size())
            {
                DWRITE_TEXT_RANGE range = { underline_pos, 1 };
                layout->SetUnderline(TRUE, range);
            }

            format->Release();
            return layout;
        }

        void MeasureString(const std::wstring& text,
            const Font& font,
            int flags,
            int* width,
            int* height)
        {
            if(width)
            {
                *width = 0;
            }
            if(height)
            {
                *height = 0;
            }
            if(text.empty())
            {
                return;
            }

            IDWriteTextLayout* layout = CreateLayout(text, font, flags,
                kUnconstrainedLayout, kUnconstrainedLayout);
            if(!layout)
            {
                return;
            }

            DWRITE_TEXT_METRICS metrics = {};
            const HRESULT hr = layout->GetMetrics(&metrics);
            layout->Release();
            if(FAILED(hr))
            {
                return;
            }

            if(width)
            {
                *width = static_cast<int>(std::ceil(
                    static_cast<double>(metrics.widthIncludingTrailingWhitespace)));
            }
            if(height)
            {
                *height = static_cast<int>(std::ceil(
                    static_cast<double>(metrics.height)));
            }
        }

    } //namespace dwrite_text

} //namespace gfx
