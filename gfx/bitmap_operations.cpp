
#include "bitmap_operations.h"

#include <vector>

#include "base/logging.h"

namespace gfx
{

    namespace
    {

        int ClampByte(int v)
        {
            if(v<0)
            {
                return 0;
            }
            if(v>255)
            {
                return 255;
            }
            return v;
        }

        uint32 PackPixel(int a, int r, int g, int b)
        {
            return Color::MakeARGB(
                static_cast<uint8>(ClampByte(a)),
                static_cast<uint8>(ClampByte(r)),
                static_cast<uint8>(ClampByte(g)),
                static_cast<uint8>(ClampByte(b)));
        }

        const uint32* RowAt(const uint8* pixels, int stride, int y)
        {
            return reinterpret_cast<const uint32*>(pixels +
                static_cast<size_t>(y)*stride);
        }

        uint32* MutableRowAt(uint8* pixels, int stride, int y)
        {
            return reinterpret_cast<uint32*>(pixels +
                static_cast<size_t>(y)*stride);
        }

    }

    Bitmap BitmapOperations::CreateDebugBitmap()
    {
        const int kSize = 32;
        const int stride = kSize * 4;
        std::vector<uint8> pixels(static_cast<size_t>(stride)*kSize, 0);
        const uint32 red = Color::MakeARGB(255, 255, 0, 0);
        for(int y=0; y<kSize; ++y)
        {
            uint32* row = MutableRowAt(&pixels[0], stride, y);
            for(int x=0; x<kSize; ++x)
            {
                row[x] = red;
            }
        }
        return Bitmap::CreateFromPixels(kSize, kSize, stride, pixels);
    }

    // static
    Bitmap BitmapOperations::CreateBlendedBitmap(
        const Bitmap& first_bitmap,
        const Bitmap& second_bitmap,
        double alpha)
    {
        DCHECK(!first_bitmap.IsNull() && !second_bitmap.IsNull());
        DCHECK((alpha>=0) && (alpha<=1));
        DCHECK(first_bitmap.Width()==second_bitmap.Width());
        DCHECK(first_bitmap.Height()==second_bitmap.Height());

        static const double alpha_min = 1.0 / 255;
        static const double alpha_max = 254.0 / 255;
        if(alpha < alpha_min)
        {
            return first_bitmap;
        }
        else if(alpha > alpha_max)
        {
            return second_bitmap;
        }

        const int width = first_bitmap.Width();
        const int height = first_bitmap.Height();
        const uint8* first_px = first_bitmap.GetPixels();
        const uint8* second_px = second_bitmap.GetPixels();
        const int first_stride = first_bitmap.Stride();
        const int second_stride = second_bitmap.Stride();
        if(!first_px || !second_px || first_stride<width*4 ||
            second_stride<width*4)
        {
            return first_bitmap;
        }

        const int dest_stride = width * 4;
        std::vector<uint8> dest(static_cast<size_t>(dest_stride)*height, 0);
        const double first_alpha = 1 - alpha;

        for(int y=0; y<height; ++y)
        {
            const uint32* first_row = RowAt(first_px, first_stride, y);
            const uint32* second_row = RowAt(second_px, second_stride, y);
            uint32* dst_row = MutableRowAt(&dest[0], dest_stride, y);
            for(int x=0; x<width; ++x)
            {
                Color first_pixel;
                Color second_pixel;
                first_pixel.SetValue(first_row[x]);
                second_pixel.SetValue(second_row[x]);
                dst_row[x] = PackPixel(
                    static_cast<int>((first_pixel.GetA()*first_alpha) +
                        (second_pixel.GetA()*alpha)),
                    static_cast<int>((first_pixel.GetR()*first_alpha) +
                        (second_pixel.GetR()*alpha)),
                    static_cast<int>((first_pixel.GetG()*first_alpha) +
                        (second_pixel.GetG()*alpha)),
                    static_cast<int>((first_pixel.GetB()*first_alpha) +
                        (second_pixel.GetB()*alpha)));
            }
        }

        return Bitmap::CreateFromPixels(width, height, dest_stride, dest);
    }

    // static
    Bitmap BitmapOperations::CreateButtonBackground(const Color& color,
        const Bitmap& image_bitmap,
        const Bitmap& mask_bitmap)
    {
        DCHECK(!image_bitmap.IsNull() && !mask_bitmap.IsNull());
        const uint8* image_px = image_bitmap.GetPixels();
        const uint8* mask_px = mask_bitmap.GetPixels();
        const int image_w = image_bitmap.Width();
        const int image_h = image_bitmap.Height();
        const int mask_w = mask_bitmap.Width();
        const int mask_h = mask_bitmap.Height();
        const int image_stride = image_bitmap.Stride();
        const int mask_stride = mask_bitmap.Stride();
        if(!image_px || !mask_px || image_w<=0 || image_h<=0 ||
            mask_w<=0 || mask_h<=0 || image_stride<image_w*4 ||
            mask_stride<mask_w*4)
        {
            return Bitmap();
        }

        const int dest_stride = mask_w * 4;
        std::vector<uint8> dest(static_cast<size_t>(dest_stride)*mask_h, 0);
        const double bg_a = color.GetA();
        const double bg_r = color.GetR();
        const double bg_g = color.GetG();
        const double bg_b = color.GetB();

        for(int y=0; y<mask_h; ++y)
        {
            uint32* dst_row = MutableRowAt(&dest[0], dest_stride, y);
            const uint32* image_row = RowAt(image_px, image_stride, y%image_h);
            const uint32* mask_row = RowAt(mask_px, mask_stride, y);
            for(int x=0; x<mask_w; ++x)
            {
                Color clr_image_pixel;
                Color clr_mask_pixel;
                clr_image_pixel.SetValue(image_row[x%image_w]);
                clr_mask_pixel.SetValue(mask_row[x]);

                const double img_a = clr_image_pixel.GetA();
                const double img_r = clr_image_pixel.GetR();
                const double img_g = clr_image_pixel.GetG();
                const double img_b = clr_image_pixel.GetB();
                const double img_alpha = img_a / 255.0;
                const double img_inv = 1 - img_alpha;
                const double mask_a = static_cast<double>(
                    clr_mask_pixel.GetA()) / 255.0;

                double out_a = (bg_a+img_a)*mask_a;
                if(out_a>255.0)
                {
                    out_a = 255.0;
                }
                dst_row[x] = PackPixel(
                    static_cast<int>(out_a),
                    static_cast<int>(((bg_r*img_inv)+(img_r*img_alpha))*mask_a),
                    static_cast<int>(((bg_g*img_inv)+(img_g*img_alpha))*mask_a),
                    static_cast<int>(((bg_b*img_inv)+(img_b*img_alpha))*mask_a));
            }
        }

        return Bitmap::CreateFromPixels(mask_w, mask_h, dest_stride, dest);
    }

} //namespace gfx
