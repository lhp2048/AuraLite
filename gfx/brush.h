
#ifndef __brush_h__
#define __brush_h__

#pragma once

#include "color.h"
#include "point.h"

namespace gfx
{

    class Brush
    {
    public:
        enum Type
        {
            SOLID,
            LINEAR_GRADIENT,
        };

        Brush() : type_(SOLID), horizontal_(true) {}

        explicit Brush(const Color& color)
            : type_(SOLID),
              color1_(color),
              horizontal_(true) {}

        Brush(const Point& point1, const Point& point2,
            const Color& color1, const Color& color2, bool horizontal)
            : type_(LINEAR_GRADIENT),
              point1_(point1),
              point2_(point2),
              color1_(color1),
              color2_(color2),
              horizontal_(horizontal) {}

        Type type() const { return type_; }
        Color color() const { return color1_; }
        Color color1() const { return color1_; }
        Color color2() const { return color2_; }
        Point point1() const { return point1_; }
        Point point2() const { return point2_; }
        bool horizontal() const { return horizontal_; }

    private:
        Type type_;
        Point point1_;
        Point point2_;
        Color color1_;
        Color color2_;
        bool horizontal_;
    };

} //namespace gfx

#endif //__brush_h__
