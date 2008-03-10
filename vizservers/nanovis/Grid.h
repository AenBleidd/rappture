#ifndef _AXIS_H_
#define _AXIS_H_

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
    void render();
    
    void setFont(R2Fonts* font);
    void setVisible(bool visible);
    void setAxisColor(float r, float g, float b, float a);
    void setLineColor(float r, float g, float b, float a);
};

inline void Grid::setVisible(bool visible)
{
    _visible = visible;
}

inline void Grid::setAxisColor(float r, float g, float b, float a)
{
    _axisColor.SetColor(r, g, b, a);
}

inline void Grid::setLineColor(float r, float g, float b, float a)
{
    _majorColor.SetColor(r, g, b, a);
    _minorColor = _majorColor;
}

#endif 
