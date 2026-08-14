
#include "list_view.h"

#include <algorithm>

#include "gfx/canvas.h"

#include "../app/event.h"
#include "../layout/box_layout.h"

namespace view
{

    namespace
    {
        const int kItemPaddingX = 8;
        const int kItemPaddingY = 4;
        const int kMinItemHeight = 24;
    }

    class ListView::Item : public View
    {
    public:
        Item(ListView* owner, const std::wstring& text)
            : owner_(owner),
              text_(text),
              selected_(false)
        {
            SetFocusable(false);
        }

        void SetText(const std::wstring& text)
        {
            text_ = text;
            SchedulePaint();
        }

        const std::wstring& text() const { return text_; }

        void SetSelected(bool selected)
        {
            if(selected_ == selected)
            {
                return;
            }
            selected_ = selected;
            SchedulePaint();
        }

        bool selected() const { return selected_; }

        virtual gfx::Size GetPreferredSize()
        {
            const int h = std::max(kMinItemHeight,
                owner_->font().GetHeight() + kItemPaddingY * 2);
            const int w = owner_->font().GetStringWidth(text_) + kItemPaddingX * 2;
            return gfx::Size(w, h);
        }

        virtual void Paint(gfx::Canvas* canvas)
        {
            View::Paint(canvas);
            if(selected_)
            {
                canvas->FillRectInt(owner_->selected_bg_, 0, 0, width(), height());
            }
            const gfx::Color& color = selected_ ? owner_->selected_text_
                                               : owner_->text_color_;
            canvas->DrawStringInt(text_, owner_->font(), color,
                kItemPaddingX, 0, std::max(0, width() - kItemPaddingX * 2),
                height(),
                gfx::Canvas::TEXT_VALIGN_MIDDLE | gfx::Canvas::NO_ELLIPSIS);
        }

        virtual bool OnMousePressed(const MouseEvent& event)
        {
            if(!event.IsOnlyLeftMouseButton())
            {
                return false;
            }
            owner_->SelectFromItem(this);
            return true;
        }

        virtual std::string GetClassName() const
        {
            return "view/ListViewItem";
        }

    private:
        ListView* owner_;
        std::wstring text_;
        bool selected_;

        DISALLOW_COPY_AND_ASSIGN(Item);
    };

    // static
    const char ListView::kViewClassName[] = "view/ListView";

    ListView::ListView()
        : text_color_(20, 20, 20),
          selected_bg_(51, 120, 210),
          selected_text_(255, 255, 255),
          selected_index_(-1),
          listener_(NULL)
    {
        SetFocusable(false);
        font_ = gfx::Font(L"Microsoft YaHei UI", 14);
        SetLayoutManager(new BoxLayout(BoxLayout::kVertical, 0, 0, 0));
    }

    ListView::ListView(ListViewListener* listener)
        : text_color_(20, 20, 20),
          selected_bg_(51, 120, 210),
          selected_text_(255, 255, 255),
          selected_index_(-1),
          listener_(listener)
    {
        SetFocusable(false);
        font_ = gfx::Font(L"Microsoft YaHei UI", 14);
        SetLayoutManager(new BoxLayout(BoxLayout::kVertical, 0, 0, 0));
    }

    ListView::~ListView() {}

    int ListView::AddItem(const std::wstring& text)
    {
        Item* item = new Item(this, text);
        items_.push_back(item);
        AddChildView(item);
        PreferredSizeChanged();
        return static_cast<int>(items_.size()) - 1;
    }

    void ListView::ClearItems()
    {
        selected_index_ = -1;
        items_.clear();
        RemoveAllChildViews(true);
        PreferredSizeChanged();
        SchedulePaint();
    }

    int ListView::item_count() const
    {
        return static_cast<int>(items_.size());
    }

    void ListView::SetSelectedIndex(int index)
    {
        if(index < -1 || index >= item_count())
        {
            return;
        }
        if(selected_index_ == index)
        {
            return;
        }
        if(selected_index_ >= 0 && selected_index_ < item_count())
        {
            items_[selected_index_]->SetSelected(false);
        }
        selected_index_ = index;
        if(selected_index_ >= 0)
        {
            items_[selected_index_]->SetSelected(true);
        }
        NotifySelection();
        SchedulePaint();
    }

    void ListView::SetFont(const gfx::Font& font)
    {
        font_ = font;
        PreferredSizeChanged();
        SchedulePaint();
    }

    void ListView::SetTextColor(const gfx::Color& color)
    {
        text_color_ = color;
        SchedulePaint();
    }

    void ListView::SetSelectedColors(const gfx::Color& background,
        const gfx::Color& text)
    {
        selected_bg_ = background;
        selected_text_ = text;
        SchedulePaint();
    }

    gfx::Size ListView::GetPreferredSize()
    {
        int w = 0;
        int h = 0;
        for(size_t i = 0; i < items_.size(); ++i)
        {
            const gfx::Size sz = items_[i]->GetPreferredSize();
            w = std::max(w, sz.width());
            h += sz.height();
        }
        return gfx::Size(w, h);
    }

    std::string ListView::GetClassName() const
    {
        return kViewClassName;
    }

    AccessibilityTypes::Role ListView::GetAccessibleRole()
    {
        return AccessibilityTypes::ROLE_OUTLINE;
    }

    void ListView::NotifySelection()
    {
        if(listener_)
        {
            listener_->ListSelectionChanged(this, selected_index_);
        }
    }

    void ListView::SelectFromItem(Item* item)
    {
        for(size_t i = 0; i < items_.size(); ++i)
        {
            if(items_[i] == item)
            {
                SetSelectedIndex(static_cast<int>(i));
                return;
            }
        }
    }

} //namespace view
