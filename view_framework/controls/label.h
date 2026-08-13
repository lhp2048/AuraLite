
#ifndef __label_h__
#define __label_h__

#pragma once

#include <string>

#include "gfx/color.h"
#include "gfx/font.h"

#include "../view.h"

namespace view
{

    // Simple text label. Not focusable by default.
    class Label : public View
    {
    public:
        static const char kViewClassName[];

        enum TextAlignment
        {
            ALIGN_LEFT,
            ALIGN_CENTER,
            ALIGN_RIGHT
        };

        Label();
        explicit Label(const std::wstring& text);
        virtual ~Label();

        void SetText(const std::wstring& text);
        const std::wstring& GetText() const { return text_; }

        void SetFont(const gfx::Font& font);
        const gfx::Font& font() const { return font_; }

        void SetColor(const gfx::Color& color);
        const gfx::Color& color() const { return color_; }

        void SetTextAlignment(TextAlignment alignment);
        TextAlignment text_alignment() const { return alignment_; }

        void SetMultiLine(bool multi_line);
        bool multi_line() const { return multi_line_; }

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual void Paint(gfx::Canvas* canvas);
        virtual std::string GetClassName() const;

    private:
        int CanvasAlignFlags() const;

        std::wstring text_;
        gfx::Font font_;
        gfx::Color color_;
        TextAlignment alignment_;
        bool multi_line_;

        DISALLOW_COPY_AND_ASSIGN(Label);
    };

} //namespace view

#endif //__label_h__
