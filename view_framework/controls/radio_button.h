#ifndef __radio_button_h__
#define __radio_button_h__

#pragma once

#include <string>

#include "gfx/color.h"
#include "gfx/font.h"

#include "button/custom_button.h"

namespace view
{

    class RadioButton;

    class RadioButtonListener
    {
    public:
        virtual void RadioButtonSelected(RadioButton* sender) = 0;

    protected:
        virtual ~RadioButtonListener() {}
    };

    // Circular radio with optional label. Same group_id siblings are exclusive.
    class RadioButton : public CustomButton
    {
    public:
        static const char kViewClassName[];

        RadioButton(const std::wstring& label, int group_id);
        RadioButton(RadioButtonListener* listener,
            const std::wstring& label, int group_id);
        virtual ~RadioButton();

        void SetChecked(bool checked);
        bool checked() const { return checked_; }

        int group_id() const { return group_id_; }

        void SetLabel(const std::wstring& label);
        const std::wstring& label() const { return label_; }

        void SetFont(const gfx::Font& font);
        void SetRadioButtonListener(RadioButtonListener* listener)
        {
            radio_listener_ = listener;
        }
        void SetTextColor(const gfx::Color& color);
        void SetDotColor(const gfx::Color& color);

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual void Paint(gfx::Canvas* canvas);
        virtual std::string GetClassName() const;
        virtual AccessibilityTypes::Role GetAccessibleRole();
        virtual AccessibilityTypes::State GetAccessibleState();

    protected:
        virtual void NotifyClick(const Event& event);

    private:
        void UncheckSiblings();

        static const int kDotSize = 16;
        static const int kLabelGap = 8;

        bool checked_;
        int group_id_;
        std::wstring label_;
        gfx::Font font_;
        gfx::Color text_color_;
        gfx::Color dot_color_;
        RadioButtonListener* radio_listener_;

        DISALLOW_COPY_AND_ASSIGN(RadioButton);
    };

} //namespace view

#endif //__radio_button_h__
