
#ifndef __list_view_h__
#define __list_view_h__

#pragma once

#include <string>
#include <vector>

#include "gfx/color.h"
#include "gfx/font.h"

#include "../view.h"

namespace view
{

    class ListView;

    class ListViewListener
    {
    public:
        virtual void ListSelectionChanged(ListView* sender, int index) = 0;

    protected:
        virtual ~ListViewListener() {}
    };

    // Vertical single-select list. Place inside ScrollView for scrolling.
    class ListView : public View
    {
    public:
        static const char kViewClassName[];

        ListView();
        explicit ListView(ListViewListener* listener);
        virtual ~ListView();

        int AddItem(const std::wstring& text);
        void ClearItems();
        int item_count() const;

        void SetSelectedIndex(int index);
        int selected_index() const { return selected_index_; }

        void SetListener(ListViewListener* listener) { listener_ = listener; }

        void SetFont(const gfx::Font& font);
        const gfx::Font& font() const { return font_; }

        void SetTextColor(const gfx::Color& color);
        void SetSelectedColors(const gfx::Color& background,
            const gfx::Color& text);

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual std::string GetClassName() const;
        virtual AccessibilityTypes::Role GetAccessibleRole();

    private:
        class Item;

        void NotifySelection();
        void SelectFromItem(Item* item);

        gfx::Font font_;
        gfx::Color text_color_;
        gfx::Color selected_bg_;
        gfx::Color selected_text_;
        int selected_index_;
        ListViewListener* listener_;
        std::vector<Item*> items_;

        DISALLOW_COPY_AND_ASSIGN(ListView);
    };

} //namespace view

#endif //__list_view_h__
