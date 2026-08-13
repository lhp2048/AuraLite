
#include "image_view.h"

#include <algorithm>

#include "gfx/canvas.h"

namespace view
{

    // static
    const char ImageView::kViewClassName[] = "view/ImageView";

    ImageView::ImageView()
        : h_align_(ALIGN_CENTER),
          v_align_(ALIGN_MIDDLE),
          scale_type_(NO_SCALE),
          preferred_size_(16, 16)
    {
        SetFocusable(false);
    }

    ImageView::~ImageView() {}

    void ImageView::SetImage(const gfx::Bitmap& image)
    {
        image_ = image;
        PreferredSizeChanged();
        SchedulePaint();
    }

    void ImageView::SetHorizontalAlignment(Alignment align)
    {
        h_align_ = align;
        SchedulePaint();
    }

    void ImageView::SetVerticalAlignment(Alignment align)
    {
        v_align_ = align;
        SchedulePaint();
    }

    void ImageView::SetScaleType(ScaleType type)
    {
        scale_type_ = type;
        SchedulePaint();
    }

    void ImageView::SetPreferredSize(const gfx::Size& size)
    {
        preferred_size_ = size;
        PreferredSizeChanged();
    }

    gfx::Size ImageView::GetPreferredSize()
    {
        if(!image_.IsNull())
        {
            return gfx::Size(image_.Width(), image_.Height());
        }
        return preferred_size_;
    }

    void ImageView::ComputeDrawRect(int* x, int* y, int* w, int* h) const
    {
        if(!x || !y || !w || !h || image_.IsNull())
        {
            return;
        }

        int img_w = image_.Width();
        int img_h = image_.Height();
        if(scale_type_ == SCALE_ASPECT && img_w>0 && img_h>0)
        {
            const double sx = static_cast<double>(width()) / img_w;
            const double sy = static_cast<double>(height()) / img_h;
            const double s = std::min(sx, sy);
            img_w = std::max(1, static_cast<int>(img_w * s));
            img_h = std::max(1, static_cast<int>(img_h * s));
        }

        *w = img_w;
        *h = img_h;

        if(h_align_ == ALIGN_CENTER)
        {
            *x = (width() - img_w) / 2;
        }
        else if(h_align_ == ALIGN_RIGHT)
        {
            *x = width() - img_w;
        }
        else
        {
            *x = 0;
        }

        if(v_align_ == ALIGN_MIDDLE)
        {
            *y = (height() - img_h) / 2;
        }
        else if(v_align_ == ALIGN_BOTTOM)
        {
            *y = height() - img_h;
        }
        else
        {
            *y = 0;
        }
    }

    void ImageView::Paint(gfx::Canvas* canvas)
    {
        View::Paint(canvas);
        if(image_.IsNull())
        {
            return;
        }

        int x = 0, y = 0, w = 0, h = 0;
        ComputeDrawRect(&x, &y, &w, &h);
        if(w<=0 || h<=0)
        {
            return;
        }

        if(w==image_.Width() && h==image_.Height())
        {
            canvas->DrawBitmapInt(image_, x, y);
        }
        else
        {
            canvas->DrawBitmapInt(image_,
                0, 0, image_.Width(), image_.Height(),
                x, y, w, h);
        }
    }

    std::string ImageView::GetClassName() const
    {
        return kViewClassName;
    }

    AccessibilityTypes::Role ImageView::GetAccessibleRole()
    {
        return AccessibilityTypes::ROLE_GRAPHIC;
    }

} //namespace view
