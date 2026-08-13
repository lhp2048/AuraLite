
#include "switch.h"

#include <algorithm>

#include "gfx/canvas.h"

#include "../app/event.h"

namespace view
{

    // static
    const char Switch::kViewClassName[] = "view/Switch";

    Switch::Switch(const std::wstring& label)
        : CustomButton(NULL),
          on_(false),
          label_(label),
          font_(L"Microsoft YaHei UI", 14),
          text_color_(0, 0, 0),
          switch_listener_(NULL)
    {
        SetFocusable(true);
        set_animate_on_state_change(false);
    }

    Switch::Switch(SwitchListener* listener, const std::wstring& label)
        : CustomButton(NULL),
          on_(false),
          label_(label),
          font_(L"Microsoft YaHei UI", 14),
          text_color_(0, 0, 0),
          switch_listener_(listener)
    {
        SetFocusable(true);
        set_animate_on_state_change(false);
    }

    Switch::~Switch() {}

    void Switch::SetOn(bool on)
    {
        if(on_ == on)
        {
            return;
        }
        on_ = on;
        SchedulePaint();
        if(switch_listener_)
        {
            switch_listener_->SwitchChanged(this, on_);
        }
    }

    void Switch::SetLabel(const std::wstring& label)
    {
        if(label_ == label)
        {
            return;
        }
        label_ = label;
        SchedulePaint();
    }

    void Switch::SetFont(const gfx::Font& font)
    {
        font_ = font;
        SchedulePaint();
    }

    void Switch::SetTextColor(const gfx::Color& color)
    {
        text_color_ = color;
        SchedulePaint();
    }

    gfx::Size Switch::GetPreferredSize()
    {
        const int text_w = label_.empty() ? 0 : font_.GetStringWidth(label_);
        const int w = kTrackWidth + (label_.empty() ? 0 : kLabelGap + text_w);
        const int h = std::max(kTrackHeight, font_.GetHeight());
        return gfx::Size(w, h);
    }

    void Switch::Paint(gfx::Canvas* canvas)
    {
        View::Paint(canvas);

        const int track_y = (height() - kTrackHeight) / 2;
        const gfx::Color track = on_ ? gfx::Color(40, 110, 200)
                                     : gfx::Color(180, 180, 180);
        canvas->FillRectInt(track, 0, track_y, kTrackWidth, kTrackHeight);

        const int thumb_y = track_y + (kTrackHeight - kThumbSize) / 2;
        const int thumb_x = on_ ? (kTrackWidth - kThumbSize - 2) : 2;
        canvas->FillRectInt(gfx::Color(255, 255, 255),
            thumb_x, thumb_y, kThumbSize, kThumbSize);
        canvas->DrawRectInt(gfx::Color(120, 120, 120),
            thumb_x, thumb_y, kThumbSize, kThumbSize);

        if(!label_.empty())
        {
            const int text_x = kTrackWidth + kLabelGap;
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

    std::string Switch::GetClassName() const
    {
        return kViewClassName;
    }

    AccessibilityTypes::Role Switch::GetAccessibleRole()
    {
        return AccessibilityTypes::ROLE_CHECKBUTTON;
    }

    AccessibilityTypes::State Switch::GetAccessibleState()
    {
        AccessibilityTypes::State state = CustomButton::GetAccessibleState();
        if(on_)
        {
            state |= AccessibilityTypes::STATE_CHECKED;
        }
        return state;
    }

    void Switch::NotifyClick(const Event& event)
    {
        SetOn(!on_);
        CustomButton::NotifyClick(event);
    }

} //namespace view
