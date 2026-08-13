
#include "label.h"

#include <algorithm>

#include "gfx/canvas.h"

namespace view
{

    // static
    const char Label::kViewClassName[] = "view/Label";

    Label::Label()
        : color_(0, 0, 0),
          alignment_(ALIGN_LEFT),
          multi_line_(false)
    {
        SetFocusable(false);
        font_ = gfx::Font(L"Microsoft YaHei UI", 14);
    }

    Label::Label(const std::wstring& text)
        : text_(text),
          color_(0, 0, 0),
          alignment_(ALIGN_LEFT),
          multi_line_(false)
    {
        SetFocusable(false);
        font_ = gfx::Font(L"Microsoft YaHei UI", 14);
    }

    Label::~Label() {}

    void Label::SetText(const std::wstring& text)
    {
        if(text_ == text)
        {
            return;
        }
        text_ = text;
        SchedulePaint();
    }

    void Label::SetFont(const gfx::Font& font)
    {
        font_ = font;
        SchedulePaint();
    }

    void Label::SetColor(const gfx::Color& color)
    {
        color_ = color;
        SchedulePaint();
    }

    void Label::SetTextAlignment(TextAlignment alignment)
    {
        if(alignment_ == alignment)
        {
            return;
        }
        alignment_ = alignment;
        SchedulePaint();
    }

    void Label::SetMultiLine(bool multi_line)
    {
        if(multi_line_ == multi_line)
        {
            return;
        }
        multi_line_ = multi_line;
        SchedulePaint();
    }

    gfx::Size Label::GetPreferredSize()
    {
        if(text_.empty())
        {
            return gfx::Size(0, font_.GetHeight());
        }
        if(multi_line_)
        {
            // Prefer width of parent/bounds when multi-line; fall back to single-line
            // width estimate.
            const int w = std::max(width(), font_.GetStringWidth(text_));
            return gfx::Size(w, font_.GetHeight());
        }
        return gfx::Size(font_.GetStringWidth(text_), font_.GetHeight());
    }

    void Label::Paint(gfx::Canvas* canvas)
    {
        View::Paint(canvas);
        if(text_.empty())
        {
            return;
        }
        canvas->DrawStringInt(text_, font_, color_, 0, 0, width(), height(),
            CanvasAlignFlags());
    }

    std::string Label::GetClassName() const
    {
        return kViewClassName;
    }

    int Label::CanvasAlignFlags() const
    {
        int flags = gfx::Canvas::TEXT_VALIGN_MIDDLE | gfx::Canvas::NO_ELLIPSIS;
        if(multi_line_)
        {
            flags |= gfx::Canvas::MULTI_LINE | gfx::Canvas::CHARACTER_BREAK;
        }
        switch(alignment_)
        {
        case ALIGN_CENTER:
            flags |= gfx::Canvas::TEXT_ALIGN_CENTER;
            break;
        case ALIGN_RIGHT:
            flags |= gfx::Canvas::TEXT_ALIGN_RIGHT;
            break;
        case ALIGN_LEFT:
        default:
            flags |= gfx::Canvas::TEXT_ALIGN_LEFT;
            break;
        }
        return flags;
    }

} //namespace view
