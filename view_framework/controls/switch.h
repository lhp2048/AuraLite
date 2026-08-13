#ifndef __switch_h__
#define __switch_h__

#pragma once

#include <string>

#include "gfx/color.h"
#include "gfx/font.h"

#include "button/custom_button.h"

namespace view
{

    class Switch;

    class SwitchListener
    {
    public:
        virtual void SwitchChanged(Switch* sender, bool on) = 0;

    protected:
        virtual ~SwitchListener() {}
    };

    // On/off toggle with optional label (pill track + thumb).
    class Switch : public CustomButton
    {
    public:
        static const char kViewClassName[];

        explicit Switch(const std::wstring& label);
        Switch(SwitchListener* listener, const std::wstring& label);
        virtual ~Switch();

        void SetOn(bool on);
        bool is_on() const { return on_; }

        void SetLabel(const std::wstring& label);
        const std::wstring& label() const { return label_; }

        void SetFont(const gfx::Font& font);
        void SetSwitchListener(SwitchListener* listener)
        {
            switch_listener_ = listener;
        }
        void SetTextColor(const gfx::Color& color);

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual void Paint(gfx::Canvas* canvas);
        virtual std::string GetClassName() const;
        virtual AccessibilityTypes::Role GetAccessibleRole();
        virtual AccessibilityTypes::State GetAccessibleState();

    protected:
        virtual void NotifyClick(const Event& event);

    private:
        static const int kTrackWidth = 40;
        static const int kTrackHeight = 20;
        static const int kThumbSize = 16;
        static const int kLabelGap = 8;

        bool on_;
        std::wstring label_;
        gfx::Font font_;
        gfx::Color text_color_;
        SwitchListener* switch_listener_;

        DISALLOW_COPY_AND_ASSIGN(Switch);
    };

} //namespace view

#endif //__switch_h__
