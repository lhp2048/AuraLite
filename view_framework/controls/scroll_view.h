#ifndef __scroll_view_h__
#define __scroll_view_h__

#pragma once

#include "gfx/color.h"

#include "../view.h"

namespace view
{

    // Vertical-only scroll container. Owns layout of |contents|; clips to bounds.
    class ScrollView : public View
    {
    public:
        static const char kViewClassName[];

        ScrollView();
        virtual ~ScrollView();

        // Takes ownership via AddChildView. Replaces any previous contents.
        void SetContents(View* contents);
        View* contents() const { return contents_; }

        void SetScrollOffset(int offset);
        int scroll_offset() const { return scroll_offset_; }

        void SetScrollbarColor(const gfx::Color& track, const gfx::Color& thumb);

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual void Layout();
        virtual void Paint(gfx::Canvas* canvas);
        virtual bool OnMousePressed(const MouseEvent& event);
        virtual bool OnMouseDragged(const MouseEvent& event);
        virtual void OnMouseReleased(const MouseEvent& event, bool canceled);
        virtual bool OnMouseWheel(const MouseWheelEvent& event);
        virtual std::string GetClassName() const;
        virtual AccessibilityTypes::Role GetAccessibleRole();
        virtual void ChildPreferredSizeChanged(View* child);

    private:
        static const int kScrollbarWidth = 10;
        static const int kMinThumbHeight = 20;
        static const int kDefaultLineScroll = 40;

        int ContentHeight() const;
        int ViewportWidth() const;
        int ViewportHeight() const;
        int MaxScrollOffset() const;
        bool NeedsScrollbar() const;
        void ClampScrollOffset();
        void UpdateContentsBounds();
        gfx::Rect ScrollbarBounds() const;
        gfx::Rect ThumbBounds() const;
        int ScrollOffsetFromThumbY(int thumb_y) const;

        View* contents_;
        int scroll_offset_;
        bool dragging_thumb_;
        int drag_thumb_anchor_y_;
        int drag_scroll_anchor_;
        gfx::Color track_color_;
        gfx::Color thumb_color_;

        DISALLOW_COPY_AND_ASSIGN(ScrollView);
    };

} //namespace view

#endif //__scroll_view_h__
