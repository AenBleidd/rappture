/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef _GRID_H_
#define _GRID_H_

#include <R2/R2Fonts.h>
#include "Axis.h"
#include "AxisRange.h"

class RGBA {
public:
    float red, green, blue, alpha;
    RGBA(float r, float g, float b, float a) : 
	red(r), green(g), blue(b), alpha(a)
    { /*empty*/ }
    void SetColor(float r, float g, float b, float a) {
	red   = r;
	green = g;
	blue  = b;
	alpha = a;
    }
};

class Grid {
    RGBA _axisColor, _majorColor, _minorColor;
    R2Fonts* _font;
    bool _visible;

public :
    Axis xAxis;
    Axis yAxis;
    Axis zAxis;

    Grid();
    bool isVisible() const {
	return _visible;
    }
    void setVisible(bool visible) {
	_visible = visible;
    }
    void setAxisColor(float r, float g, float b, float a) {
	_axisColor.SetColor(r, g, b, a);
    }
    void setLineColor(float r, float g, float b, float a) {
	_majorColor.SetColor(r, g, b, a);
	_minorColor = _majorColor;
    }
    void render();
    void setFont(R2Fonts* font);
};


#endif 
