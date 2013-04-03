/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef CONTOURLINEFILTER_H
#define CONTOURLINEFILTER_H

#include <list>
#include <vector>

#include <graphics/Geometry.h>
#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>

namespace nv {

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
	int createLine(int width, int height, vrmath::Vector3f *vertices, bool top);
	int createLine(int width, int height, vrmath::Vector4f *vertices, bool top);

	float _value;
	std::list<vrmath::Vector3f> _points;

    private:
	bool isValueWithIn(float v1, float v2)
        {
	    return ((_value >= v1 && _value <= v2) || 
		    (_value >= v2 && _value <= v1));
	}
	void getContourPoint(int vertexIndex1, int vertexIndex2, vrmath::Vector3f *vertices, int width, bool top);
	void getContourPoint(int vertexIndex1, int vertexIndex2, vrmath::Vector4f *vertices, int width, bool top);
    };

    typedef std::list<ContourLine *> ContourLineList;
    typedef std::vector<vrmath::Vector3f> Vector3fArray;

    ContourLineFilter();

    nv::graphics::Geometry *create(float min, float max, int linecount, vrmath::Vector3f *vertices, int width, int height);
    nv::graphics::Geometry *create(float min, float max, int linecount, vrmath::Vector4f *vertices, int width, int height);

    void setColorMap(Vector3fArray *colorMap);
    
    void setHeightTop(bool top)
    {
	_top = top;
    }

private:
    void clear();

    ContourLineList _lines;
    Vector3fArray *_colorMap;
    bool _top;
};

}

#endif 
