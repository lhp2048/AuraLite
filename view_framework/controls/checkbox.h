
#ifndef __checkbox_h__
#define __checkbox_h__

#pragma once

#include <string>

#include "gfx/color.h"
#include "gfx/font.h"

#include "button/custom_button.h"

namespace view
{

    class Checkbox;

    class CheckboxListener
    {
    public:
        virtual void CheckboxChanged(Checkbox* sender, bool checked) = 0;

    protected:
        virtual ~CheckboxListener() {}
    };

    // Checkbox with optional text label. Click toggles checked state.
    class Checkbox : public CustomButton
    {
    public:
        static const char kViewClassName[];

        explicit Checkbox(const std::wstring& label);
        Checkbox(CheckboxListener* listener, const std::wstring& label);
        virtual ~Checkbox();

        void SetChecked(bool checked);
        bool checked() const { return checked_; }

        void SetLabel(const std::wstring& label);
        const std::wstring& label() const { return label_; }

        void SetFont(const gfx::Font& font);
        const gfx::Font& font() const { return font_; }

        void SetCheckboxListener(CheckboxListener* listener)
        {
            checkbox_listener_ = listener;
        }

        void SetTextColor(const gfx::Color& color);
        void SetBoxColor(const gfx::Color& color);

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual void Paint(gfx::Canvas* canvas);
        virtual std::string GetClassName() const;
        virtual AccessibilityTypes::Role GetAccessibleRole();
        virtual AccessibilityTypes::State GetAccessibleState();

    protected:
        // Overridden from CustomButton / Button:
        virtual void NotifyClick(const Event& event);

    private:
        static const int kBoxSize = 16;
        static const int kLabelGap = 8;

        bool checked_;
        std::wstring label_;
        gfx::Font font_;
        gfx::Color text_color_;
        gfx::Color box_color_;
        CheckboxListener* checkbox_listener_;

        DISALLOW_COPY_AND_ASSIGN(Checkbox);
    };

} //namespace view

#endif //__checkbox_h__
