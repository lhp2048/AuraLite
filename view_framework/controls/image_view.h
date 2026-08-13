#ifndef __image_view_h__
#define __image_view_h__

#pragma once

#include "gfx/bitmap.h"
#include "gfx/color.h"

#include "../view.h"

namespace view
{

    // Non-interactive image display. Prefer ImageButton when clickable.
    class ImageView : public View
    {
    public:
        static const char kViewClassName[];

        enum Alignment
        {
            ALIGN_LEFT = 0,
            ALIGN_CENTER,
            ALIGN_RIGHT,
            ALIGN_TOP = ALIGN_LEFT,
            ALIGN_MIDDLE = ALIGN_CENTER,
            ALIGN_BOTTOM = ALIGN_RIGHT
        };

        enum ScaleType
        {
            // Draw at intrinsic size (clipped by view bounds).
            NO_SCALE,
            // Fit inside bounds preserving aspect ratio.
            SCALE_ASPECT
        };

        ImageView();
        virtual ~ImageView();

        void SetImage(const gfx::Bitmap& image);
        const gfx::Bitmap& image() const { return image_; }

        void SetHorizontalAlignment(Alignment align);
        void SetVerticalAlignment(Alignment align);
        void SetScaleType(ScaleType type);

        // Optional preferred size when image is null.
        void SetPreferredSize(const gfx::Size& size);

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual void Paint(gfx::Canvas* canvas);
        virtual std::string GetClassName() const;
        virtual AccessibilityTypes::Role GetAccessibleRole();

    private:
        void ComputeDrawRect(int* x, int* y, int* w, int* h) const;

        gfx::Bitmap image_;
        Alignment h_align_;
        Alignment v_align_;
        ScaleType scale_type_;
        gfx::Size preferred_size_;

        DISALLOW_COPY_AND_ASSIGN(ImageView);
    };

} //namespace view

#endif //__image_view_h__
