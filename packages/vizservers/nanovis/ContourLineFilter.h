/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef CONTOURLINEFILTER_H
#define CONTOURLINEFILTER_H

#include <list>

#include <graphics/Geometry.h>

#include "Vector3.h"
#include "Vector4.h"

class ContourLineFilter
{
public:
    class ContourLine
    {
    public:
	ContourLine(float value);

        /**
	 * @return Returns the number of points
	 */
	int createLine(int width, int height, Vector3 *vertices, bool top);
	int createLine(int width, int height, Vector4 *vertices, bool top);

	float _value;
	std::list<Vector3> _points;

    private:
	bool isValueWithIn(float v1, float v2)
        {
	    return ((_value >= v1 && _value <= v2) || 
		    (_value >= v2 && _value <= v1));
	}
	void getContourPoint(int vertexIndex1, int vertexIndex2, Vector3 *vertices, int width, bool top);
	void getContourPoint(int vertexIndex1, int vertexIndex2, Vector4 *vertices, int width, bool top);
    };

    typedef std::list<ContourLine *> ContourLineList;

    ContourLineFilter();

    nv::graphics::Geometry *create(float min, float max, int linecount, Vector3 *vertices, int width, int height);
    nv::graphics::Geometry *create(float min, float max, int linecount, Vector4 *vertices, int width, int height);

    void setColorMap(Vector3Array *colorMap);
    
    void setHeightTop(bool top)
    {
	_top = top;
    }

private:
    void clear();

    ContourLineList _lines;
    Vector3Array *_colorMap;
    bool _top;
};

#endif 
