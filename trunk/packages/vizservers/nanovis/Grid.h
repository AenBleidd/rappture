/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_GRID_H
#define NV_GRID_H

#include <util/Fonts.h>

#include "Axis.h"
#include "AxisRange.h"

namespace nv {

class RGBA
{
public:
    RGBA(float r, float g, float b, float a) :
        red(r),
        green(g),
        blue(b),
        alpha(a)
    {}

    void set(float r, float g, float b, float a)
    {
        red   = r;
        green = g;
        blue  = b;
        alpha = a;
    }

    float red, green, blue, alpha;
};

class Grid
{
public:
    Grid();
    virtual ~Grid();

    bool isVisible() const
    {
        return _visible;
    }

    void setVisible(bool visible)
    {
        _visible = visible;
    }

    void setAxisColor(float r, float g, float b, float a)
    {
        _axisColor.set(r, g, b, a);
    }

    void setLineColor(float r, float g, float b, float a)
    {
        _majorColor.set(r, g, b, a);
        _minorColor = _majorColor;
    }

    void render();

    void setFont(nv::util::Fonts *font);

    Axis xAxis;
    Axis yAxis;
    Axis zAxis;

private:
    RGBA _axisColor, _majorColor, _minorColor;
    nv::util::Fonts *_font;
    bool _visible;
};

}

#endif 
