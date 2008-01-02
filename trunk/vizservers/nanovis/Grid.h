#ifndef _AXIS_H_
#define _AXIS_H_

#include <R2/R2Fonts.h>
#include "Vector3.h"
#include "Vector4.h"

class Grid {
	Vector3 __axisScale;

	Vector4 _axisColor;
	Vector4 _gridLineColor;
	Vector3 _axisMax;
	Vector3 _axisMin;
	Vector3 _axisGridLineCount;
	R2Fonts* _font;
	bool _showGrid;
	bool _visible;

    R2string _axisName[3];

public :
	Grid();
public :
	bool isVisible() const; 
	void render();

	void setMin(const Vector3& min);
	void setMax(const Vector3& max);

	void setFont(R2Fonts* font);
	void setVisible(bool visible);

    void setGridLineCount(int x, int y, int z);
    void setAxisColor(float r, float g, float b, float a);
    void setGridLineColor(float r, float g, float b, float a);
    void setMinMax(const Vector3& min, const Vector3& max);
    void setAxisName(int axisID, const char* name);
};

inline bool Grid::isVisible() const
{
	return _visible;
}

inline void Grid::setMin(const Vector3& min)
{
	_axisMin = min;
	__axisScale = _axisMax - _axisMin;
}

inline void Grid::setMax(const Vector3& max)
{
	_axisMax = max;
	__axisScale = _axisMax - _axisMin;
}

inline void Grid::setVisible(bool visible)
{
	_visible = visible;
}

inline void Grid::setGridLineCount(int x, int y, int z)
{
    _axisGridLineCount.x = x;
    _axisGridLineCount.y = y;
    _axisGridLineCount.z = z;
}

inline void Grid::setAxisColor(float r, float g, float b, float a)
{
    _axisColor.x = r;
    _axisColor.y = g;
    _axisColor.z = b;
    _axisColor.w = a;
}

inline void Grid::setGridLineColor(float r, float g, float b, float a)
{
    _gridLineColor.x = r;
    _gridLineColor.y = g;
    _gridLineColor.z = b;
    _gridLineColor.w = a;
}

inline void Grid::setMinMax(const Vector3& min, const Vector3& max)
{
    _axisMin = min;
    _axisMax = max;
	__axisScale.x = _axisMax.x - _axisMin.x;
	__axisScale.y = _axisMax.y - _axisMin.y;
	__axisScale.z = _axisMax.z - _axisMin.z;
}

inline void Grid::setAxisName(int axisID, const char* name)
{
    if (axisID >= 0 && axisID < 3)
    {
        _axisName[axisID] = name;
    }
}


#endif 
