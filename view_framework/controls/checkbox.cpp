
#include "checkbox.h"

#include <algorithm>

#include "gfx/canvas.h"

#include "../app/event.h"

namespace view
{

    // static
    const char Checkbox::kViewClassName[] = "view/Checkbox";

    Checkbox::Checkbox(const std::wstring& label)
        : CustomButton(NULL),
          checked_(false),
          label_(label),
          font_(L"Microsoft YaHei UI", 14),
          text_color_(0, 0, 0),
          box_color_(60, 60, 60),
          checkbox_listener_(NULL)
    {
        SetFocusable(true);
        set_animate_on_state_change(false);
    }

    Checkbox::Checkbox(CheckboxListener* listener, const std::wstring& label)
        : CustomButton(NULL),
          checked_(false),
          label_(label),
          font_(L"Microsoft YaHei UI", 14),
          text_color_(0, 0, 0),
          box_color_(60, 60, 60),
          checkbox_listener_(listener)
    {
        SetFocusable(true);
        set_animate_on_state_change(false);
    }

    Checkbox::~Checkbox() {}

    void Checkbox::SetChecked(bool checked)
    {
        if(checked_ == checked)
        {
            return;
        }
        checked_ = checked;
        SchedulePaint();
        if(checkbox_listener_)
        {
            checkbox_listener_->CheckboxChanged(this, checked_);
        }
    }

    void Checkbox::SetLabel(const std::wstring& label)
    {
        if(label_ == label)
        {
            return;
        }
        label_ = label;
        SchedulePaint();
    }

    void Checkbox::SetFont(const gfx::Font& font)
    {
        font_ = font;
        SchedulePaint();
    }

    void Checkbox::SetTextColor(const gfx::Color& color)
    {
        text_color_ = color;
        SchedulePaint();
    }

    void Checkbox::SetBoxColor(const gfx::Color& color)
    {
        box_color_ = color;
        SchedulePaint();
    }

    gfx::Size Checkbox::GetPreferredSize()
    {
        const int text_w = label_.empty() ? 0 : font_.GetStringWidth(label_);
        const int w = kBoxSize + (label_.empty() ? 0 : kLabelGap + text_w);
        const int h = std::max(kBoxSize, font_.GetHeight());
        return gfx::Size(w, h);
    }

    void Checkbox::Paint(gfx::Canvas* canvas)
    {
        View::Paint(canvas);

        const int box_y = (height() - kBoxSize) / 2;
        canvas->DrawRectInt(box_color_, 0, box_y, kBoxSize, kBoxSize);

        if(checked_)
        {
            // Simple check mark via two lines.
            const gfx::Color mark(20, 120, 60);
            canvas->FillRectInt(mark, 3, box_y + 7, 3, 2);
            canvas->FillRectInt(mark, 5, box_y + 9, 2, 2);
            canvas->FillRectInt(mark, 6, box_y + 7, 2, 2);
            canvas->FillRectInt(mark, 7, box_y + 5, 2, 2);
            canvas->FillRectInt(mark, 8, box_y + 3, 2, 2);
        }

        if(!label_.empty())
        {
            const int text_x = kBoxSize + kLabelGap;
            canvas->DrawStringInt(label_, font_, text_color_,
                text_x, 0, width() - text_x, height(),
                gfx::Canvas::TEXT_ALIGN_LEFT |
                gfx::Canvas::TEXT_VALIGN_MIDDLE |
                gfx::Canvas::NO_ELLIPSIS);
        }

        if(HasFocus())
        {
            canvas->DrawFocusRect(0, 0, width(), height());
        }
    }

    std::string Checkbox::GetClassName() const
    {
        return kViewClassName;
    }

    AccessibilityTypes::Role Checkbox::GetAccessibleRole()
    {
        return AccessibilityTypes::ROLE_CHECKBUTTON;
    }

    AccessibilityTypes::State Checkbox::GetAccessibleState()
    {
        AccessibilityTypes::State state = CustomButton::GetAccessibleState();
        if(checked_)
        {
            state |= AccessibilityTypes::STATE_CHECKED;
        }
        return state;
    }

    void Checkbox::NotifyClick(const Event& event)
    {
        SetChecked(!checked_);
        CustomButton::NotifyClick(event);
    }

} //namespace view
