
#include "radio_button.h"

#include <algorithm>

#include "gfx/canvas.h"

#include "../app/event.h"

namespace view
{

    // static
    const char RadioButton::kViewClassName[] = "view/RadioButton";

    RadioButton::RadioButton(const std::wstring& label, int group_id)
        : CustomButton(NULL),
          checked_(false),
          group_id_(group_id),
          label_(label),
          font_(L"Microsoft YaHei UI", 14),
          text_color_(0, 0, 0),
          dot_color_(60, 60, 60),
          radio_listener_(NULL)
    {
        SetFocusable(true);
        set_animate_on_state_change(false);
    }

    RadioButton::RadioButton(RadioButtonListener* listener,
        const std::wstring& label, int group_id)
        : CustomButton(NULL),
          checked_(false),
          group_id_(group_id),
          label_(label),
          font_(L"Microsoft YaHei UI", 14),
          text_color_(0, 0, 0),
          dot_color_(60, 60, 60),
          radio_listener_(listener)
    {
        SetFocusable(true);
        set_animate_on_state_change(false);
    }

    RadioButton::~RadioButton() {}

    void RadioButton::SetChecked(bool checked)
    {
        if(checked_ == checked)
        {
            return;
        }
        checked_ = checked;
        if(checked_)
        {
            UncheckSiblings();
        }
        SchedulePaint();
        if(checked_ && radio_listener_)
        {
            radio_listener_->RadioButtonSelected(this);
        }
    }

    void RadioButton::SetLabel(const std::wstring& label)
    {
        if(label_ == label)
        {
            return;
        }
        label_ = label;
        SchedulePaint();
    }

    void RadioButton::SetFont(const gfx::Font& font)
    {
        font_ = font;
        SchedulePaint();
    }

    void RadioButton::SetTextColor(const gfx::Color& color)
    {
        text_color_ = color;
        SchedulePaint();
    }

    void RadioButton::SetDotColor(const gfx::Color& color)
    {
        dot_color_ = color;
        SchedulePaint();
    }

    gfx::Size RadioButton::GetPreferredSize()
    {
        const int text_w = label_.empty() ? 0 : font_.GetStringWidth(label_);
        const int w = kDotSize + (label_.empty() ? 0 : kLabelGap + text_w);
        const int h = std::max(kDotSize, font_.GetHeight());
        return gfx::Size(w, h);
    }

    void RadioButton::Paint(gfx::Canvas* canvas)
    {
        View::Paint(canvas);

        const int cy = height() / 2;
        const int r = kDotSize / 2;
        // Approximate circle with concentric rects (no ellipse API on Canvas).
        canvas->DrawRectInt(dot_color_, 0, cy - r, kDotSize, kDotSize);
        canvas->DrawRectInt(dot_color_, 1, cy - r + 1, kDotSize - 2, kDotSize - 2);
        if(checked_)
        {
            const gfx::Color fill(40, 110, 200);
            canvas->FillRectInt(fill, 4, cy - r + 4, kDotSize - 8, kDotSize - 8);
        }

        if(!label_.empty())
        {
            const int text_x = kDotSize + kLabelGap;
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

    std::string RadioButton::GetClassName() const
    {
        return kViewClassName;
    }

    AccessibilityTypes::Role RadioButton::GetAccessibleRole()
    {
        return AccessibilityTypes::ROLE_RADIOBUTTON;
    }

    AccessibilityTypes::State RadioButton::GetAccessibleState()
    {
        AccessibilityTypes::State state = CustomButton::GetAccessibleState();
        if(checked_)
        {
            state |= AccessibilityTypes::STATE_CHECKED;
        }
        return state;
    }

    void RadioButton::NotifyClick(const Event& event)
    {
        SetChecked(true);
        CustomButton::NotifyClick(event);
    }

    void RadioButton::UncheckSiblings()
    {
        View* parent = GetParent();
        if(!parent)
        {
            return;
        }
        for(int i = 0; i < parent->GetChildViewCount(); ++i)
        {
            View* child = parent->GetChildViewAt(i);
            if(child == this || child->GetClassName() != kViewClassName)
            {
                continue;
            }
            RadioButton* radio = static_cast<RadioButton*>(child);
            if(radio->group_id() == group_id_ && radio->checked())
            {
                radio->checked_ = false;
                radio->SchedulePaint();
            }
        }
    }

} //namespace view
