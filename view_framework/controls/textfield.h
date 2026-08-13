#ifndef __textfield_h__
#define __textfield_h__

#pragma once

#include <string>

#include "gfx/color.h"
#include "gfx/font.h"

#include "../view.h"

namespace view
{

    class KeyEvent;
    class Textfield;

    class TextfieldController
    {
    public:
        virtual void ContentsChanged(Textfield* sender,
            const std::wstring& new_contents) {}
        // Return true if the key was handled.
        virtual bool HandleKeyEvent(Textfield* sender, const KeyEvent& key_event)
        {
            return false;
        }

    protected:
        virtual ~TextfieldController() {}
    };

    // Single-line text input. Supports password mode (displays bullets).
    // IME composition is not fully supported in this first version; ASCII /
    // mapped virtual keys via ToUnicode work for basic input.
    class Textfield : public View,
                      public ContextMenuController
    {
    public:
        static const char kViewClassName[];

        enum StyleFlags
        {
            STYLE_DEFAULT = 0,
            STYLE_PASSWORD = 1 << 0
        };

        enum EditCommand
        {
            EDIT_CUT = 1,
            EDIT_COPY = 2,
            EDIT_PASTE = 3,
            EDIT_SELECT_ALL = 4
        };

        Textfield();
        explicit Textfield(StyleFlags style);
        virtual ~Textfield();

        void SetText(const std::wstring& text);
        const std::wstring& text() const { return text_; }

        void SelectAll();
        void ClearSelection();
        bool HasSelection() const;

        bool Cut();
        bool Copy() const;
        bool Paste();

        // When true (default), right-click shows cut/copy/paste/select-all.
        void SetContextMenuEnabled(bool enabled);

        void SetController(TextfieldController* controller)
        {
            controller_ = controller;
        }
        TextfieldController* controller() const { return controller_; }

        void SetReadOnly(bool read_only);
        bool read_only() const { return read_only_; }

        void SetFont(const gfx::Font& font);
        const gfx::Font& font() const { return font_; }

        void SetTextColor(const gfx::Color& color);
        void SetBackgroundColor(const gfx::Color& color);
        void SetBorderColor(const gfx::Color& color);

        bool IsPassword() const
        {
            return (style_ & STYLE_PASSWORD) != 0;
        }

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual void Paint(gfx::Canvas* canvas);
        virtual bool OnMousePressed(const MouseEvent& event);
        virtual bool OnMouseDragged(const MouseEvent& event);
        virtual bool OnKeyPressed(const KeyEvent& event);
        virtual void OnFocus();
        virtual void OnBlur();
        virtual std::string GetClassName() const;
        virtual AccessibilityTypes::Role GetAccessibleRole();

        // Overridden from ContextMenuController:
        virtual void ShowContextMenu(View* source,
            const gfx::Point& p,
            bool is_mouse_gesture);

    private:
        std::wstring DisplayText() const;
        size_t HitTestCursor(int local_x) const;
        void GetOrderedSelection(size_t* start, size_t* end) const;
        void SetCursor(size_t pos, bool extend_selection);
        void DeleteSelection();
        void InsertChar(wchar_t ch);
        void InsertText(const std::wstring& text);
        void DeleteSelectionOrChar(bool forward);
        void NotifyContentsChanged();
        bool MapKeyToChar(const KeyEvent& event, wchar_t* out) const;
        bool CopyToClipboard() const;
        bool PasteFromClipboard();
        bool CutToClipboard();
        void BuildEditMenuModel(class MenuModel* model) const;

        int style_;
        std::wstring text_;
        // selection_start_ is the anchor; cursor_pos_ is the active end.
        size_t selection_start_;
        size_t cursor_pos_;
        bool read_only_;
        bool context_menu_enabled_;
        gfx::Font font_;
        gfx::Color text_color_;
        gfx::Color background_color_;
        gfx::Color border_color_;
        TextfieldController* controller_;

        DISALLOW_COPY_AND_ASSIGN(Textfield);
    };

} //namespace view

#endif //__textfield_h__
