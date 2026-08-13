
#include "scroll_view.h"

#include <algorithm>

#include "gfx/canvas.h"

#include "../app/event.h"

namespace view
{

    // static
    const char ScrollView::kViewClassName[] = "view/ScrollView";

    ScrollView::ScrollView()
        : contents_(NULL),
          scroll_offset_(0),
          dragging_thumb_(false),
          drag_thumb_anchor_y_(0),
          drag_scroll_anchor_(0),
          track_color_(220, 220, 220),
          thumb_color_(150, 150, 150)
    {
        SetFocusable(false);
    }

    ScrollView::~ScrollView() {}

    void ScrollView::SetContents(View* contents)
    {
        if(contents_ == contents)
        {
            return;
        }
        if(contents_)
        {
            RemoveChildView(contents_);
            delete contents_;
            contents_ = NULL;
        }
        contents_ = contents;
        if(contents_)
        {
            AddChildView(contents_);
        }
        scroll_offset_ = 0;
        Layout();
        SchedulePaint();
    }

    void ScrollView::SetScrollOffset(int offset)
    {
        const int old = scroll_offset_;
        scroll_offset_ = offset;
        ClampScrollOffset();
        if(old == scroll_offset_)
        {
            return;
        }
        UpdateContentsBounds();
        SchedulePaint();
    }

    void ScrollView::SetScrollbarColor(const gfx::Color& track,
        const gfx::Color& thumb)
    {
        track_color_ = track;
        thumb_color_ = thumb;
        SchedulePaint();
    }

    gfx::Size ScrollView::GetPreferredSize()
    {
        if(contents_)
        {
            const gfx::Size cs = contents_->GetPreferredSize();
            return gfx::Size(cs.width() + kScrollbarWidth, std::min(200, cs.height()));
        }
        return gfx::Size(100, 100);
    }

    void ScrollView::Layout()
    {
        ClampScrollOffset();
        UpdateContentsBounds();
    }

    void ScrollView::Paint(gfx::Canvas* canvas)
    {
        View::Paint(canvas);
        if(!NeedsScrollbar())
        {
            return;
        }

        const gfx::Rect track = ScrollbarBounds();
        canvas->FillRectInt(track_color_,
            track.x(), track.y(), track.width(), track.height());
        const gfx::Rect thumb = ThumbBounds();
        canvas->FillRectInt(thumb_color_,
            thumb.x(), thumb.y(), thumb.width(), thumb.height());
    }

    bool ScrollView::OnMousePressed(const MouseEvent& event)
    {
        if(!event.IsOnlyLeftMouseButton() || !NeedsScrollbar())
        {
            return false;
        }
        const gfx::Rect thumb = ThumbBounds();
        if(thumb.Contains(event.x(), event.y()))
        {
            dragging_thumb_ = true;
            drag_thumb_anchor_y_ = event.y();
            drag_scroll_anchor_ = scroll_offset_;
            return true;
        }
        const gfx::Rect track = ScrollbarBounds();
        if(track.Contains(event.x(), event.y()))
        {
            SetScrollOffset(ScrollOffsetFromThumbY(event.y() - thumb.height() / 2));
            dragging_thumb_ = true;
            drag_thumb_anchor_y_ = event.y();
            drag_scroll_anchor_ = scroll_offset_;
            return true;
        }
        return false;
    }

    bool ScrollView::OnMouseDragged(const MouseEvent& event)
    {
        if(!dragging_thumb_)
        {
            return false;
        }
        const int delta = event.y() - drag_thumb_anchor_y_;
        const int track_h = ViewportHeight();
        const int thumb_h = ThumbBounds().height();
        const int travel = std::max(1, track_h - thumb_h);
        const int max_scroll = MaxScrollOffset();
        const int delta_scroll = max_scroll * delta / travel;
        SetScrollOffset(drag_scroll_anchor_ + delta_scroll);
        return true;
    }

    void ScrollView::OnMouseReleased(const MouseEvent& event, bool canceled)
    {
        dragging_thumb_ = false;
    }

    bool ScrollView::OnMouseWheel(const MouseWheelEvent& event)
    {
        int line = 0;
        if(contents_)
        {
            line = contents_->GetLineScrollIncrement(this, false,
                event.GetOffset() < 0);
        }
        if(line <= 0)
        {
            line = kDefaultLineScroll;
        }
        // Windows wheel: positive offset typically means scroll up (content down).
        if(event.GetOffset() > 0)
        {
            SetScrollOffset(scroll_offset_ - line);
        }
        else if(event.GetOffset() < 0)
        {
            SetScrollOffset(scroll_offset_ + line);
        }
        return true;
    }

    std::string ScrollView::GetClassName() const
    {
        return kViewClassName;
    }

    AccessibilityTypes::Role ScrollView::GetAccessibleRole()
    {
        return AccessibilityTypes::ROLE_PANE;
    }

    void ScrollView::ChildPreferredSizeChanged(View* child)
    {
        if(child == contents_)
        {
            Layout();
            SchedulePaint();
        }
    }

    int ScrollView::ContentHeight() const
    {
        if(!contents_)
        {
            return 0;
        }
        return contents_->GetPreferredSize().height();
    }

    int ScrollView::ViewportWidth() const
    {
        return NeedsScrollbar() ? std::max(0, width() - kScrollbarWidth) : width();
    }

    int ScrollView::ViewportHeight() const
    {
        return height();
    }

    int ScrollView::MaxScrollOffset() const
    {
        return std::max(0, ContentHeight() - ViewportHeight());
    }

    bool ScrollView::NeedsScrollbar() const
    {
        return ContentHeight() > ViewportHeight();
    }

    void ScrollView::ClampScrollOffset()
    {
        scroll_offset_ = std::max(0, std::min(scroll_offset_, MaxScrollOffset()));
    }

    void ScrollView::UpdateContentsBounds()
    {
        if(!contents_)
        {
            return;
        }
        const int cw = ViewportWidth();
        const int ch = std::max(ContentHeight(), ViewportHeight());
        contents_->SetBounds(0, -scroll_offset_, cw, ch);
    }

    gfx::Rect ScrollView::ScrollbarBounds() const
    {
        return gfx::Rect(width() - kScrollbarWidth, 0, kScrollbarWidth, height());
    }

    gfx::Rect ScrollView::ThumbBounds() const
    {
        const int track_h = ViewportHeight();
        const int content_h = std::max(1, ContentHeight());
        int thumb_h = track_h * track_h / content_h;
        thumb_h = std::max(kMinThumbHeight, std::min(track_h, thumb_h));
        const int max_scroll = MaxScrollOffset();
        const int travel = std::max(0, track_h - thumb_h);
        const int thumb_y = (max_scroll > 0)
            ? (travel * scroll_offset_ / max_scroll) : 0;
        return gfx::Rect(width() - kScrollbarWidth, thumb_y,
            kScrollbarWidth, thumb_h);
    }

    int ScrollView::ScrollOffsetFromThumbY(int thumb_y) const
    {
        const int track_h = ViewportHeight();
        const int thumb_h = ThumbBounds().height();
        const int travel = std::max(1, track_h - thumb_h);
        const int y = std::max(0, std::min(thumb_y, travel));
        return MaxScrollOffset() * y / travel;
    }

} //namespace view
