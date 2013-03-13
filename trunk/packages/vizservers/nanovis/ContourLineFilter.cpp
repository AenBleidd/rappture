/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * @note : I refer to 'http://www.codeproject.com/useritems/ScoOterVisualizationPart2.asp'
 * @note : for this class
 */

#include <stdlib.h>
#include <memory.h>

#include <string>

#include <graphics/VertexBuffer.h>

#include "ContourLineFilter.h"

using namespace nv::graphics;
using namespace vrmath;

ContourLineFilter::ContourLineFilter()
    : _colorMap(0), _top(false)
{
}

void 
ContourLineFilter::clear()
{
	
    ContourLineFilter::ContourLineList::iterator iter;
    for (iter = _lines.begin(); iter != _lines.end(); ++iter) { 
	delete (*iter);
    }
    _lines.clear();	
}

Geometry * 
ContourLineFilter::create(float min, float max, int linecount, 
			  Vector3f* vertices, int width, int height)
{
    _lines.clear();
    
    float transtion = (max - min) / (linecount + 1);
    float val;
    int totalPoints = 0, numPoints;
    for (int i = 1; i <= linecount; ++i) {
	val = min + i * transtion;
	
	ContourLine* c = new ContourLine(val);
	numPoints = c->createLine(width, height, vertices, _top);
	if (numPoints != 0) {
	    totalPoints += numPoints;
	    _lines.push_back(c);
	} else {
	    delete c;
	}
    }
    Vector3f* vertexSet = (Vector3f *)malloc(sizeof(Vector3f) * totalPoints);
    Vector3f* colorSet = NULL;
    if (_colorMap) {
	colorSet = (Vector3f *)malloc(sizeof(Vector3f) * totalPoints);
    }
    ContourLineFilter::ContourLineList::iterator iter;
    unsigned int index = 0, colorIndex = 0;
    for (iter = _lines.begin(); iter != _lines.end(); ++iter, ++colorIndex) {
	std::list<Vector3f>& lines = (*iter)->_points;
	std::list<Vector3f>::iterator iter2;
	for (iter2 = lines.begin(); iter2 != lines.end(); ++iter2, ++index) {
	    if (_colorMap && (colorIndex < _colorMap->size())) {
		colorSet[index] = _colorMap->at(colorIndex);
	    } else {
		//colorSet[index].set((*iter)->_value, (*iter)->_value, (*iter)->_value);
	    }
	    vertexSet[index] = (*iter2);
	}
    }
    VertexBuffer *vertexBuffer = 
        new VertexBuffer(VertexBuffer::POSITION3, totalPoints,
                         totalPoints * sizeof(Vector3f), vertexSet, false);
    VertexBuffer *colorBuffer = NULL;
    if (_colorMap) {
        colorBuffer =
            new VertexBuffer(VertexBuffer::COLOR4, totalPoints, 
                             totalPoints * sizeof(Vector3f), colorSet, false);
    }
    Geometry *geometry = new Geometry(Geometry::LINES, vertexBuffer, colorBuffer, 0);
    clear();
    return geometry;
}

Geometry * 
ContourLineFilter::create(float min, float max, int linecount, 
			  Vector4f* vertices, int width, int height)
{
    _lines.clear();

    float transtion = (max - min) / (linecount + 1);

    float val;
    int totalPoints = 0, numPoints;
    for (int i = 1; i <= linecount; ++i) {
	val = min + i * transtion;
	
	ContourLine* c = new ContourLine(val);
	numPoints = c->createLine(width, height, vertices, _top);
	if (numPoints != 0) {
	    totalPoints += numPoints;
	    _lines.push_back(c);
	} else {
	    delete c;
	}
    }
    Vector3f* vertexSet = (Vector3f *)malloc(sizeof(Vector3f) * totalPoints);
    Vector3f* colorSet  = (Vector3f *)malloc(sizeof(Vector3f) * totalPoints);
	
    ContourLineFilter::ContourLineList::iterator iter;
    unsigned int index = 0, colorIndex = 0;
    for (iter = _lines.begin(); iter != _lines.end(); ++iter, ++colorIndex) {
	std::list<Vector3f>& lines = (*iter)->_points;
	std::list<Vector3f>::iterator iter2;
	for (iter2 = lines.begin(); iter2 != lines.end(); ++iter2, ++index) {
	    if (_colorMap && (colorIndex < _colorMap->size())) {
		colorSet[index] = _colorMap->at(colorIndex);
	    } else {
		colorSet[index].set((*iter)->_value, (*iter)->_value, 
				    (*iter)->_value);
	    }
	    vertexSet[index] = (*iter2);
	}
    }
    VertexBuffer *vertexBuffer =
        new VertexBuffer(VertexBuffer::POSITION3, totalPoints,
                         totalPoints * sizeof(Vector3f), vertexSet, false);
    VertexBuffer *colorBuffer =
        new VertexBuffer(VertexBuffer::COLOR4, totalPoints,
                         totalPoints * sizeof(Vector3f), colorSet, false);
    Geometry *geometry =
        new Geometry(Geometry::LINES, vertexBuffer, colorBuffer, 0);
    clear();
    return geometry;
}


ContourLineFilter::ContourLine::ContourLine(float value)
    : _value(value)
{
}


int 
ContourLineFilter::ContourLine::createLine(int width, int height, 
					   Vector3f* vertices, bool top)
{
    _points.clear();

    int hl = height - 1;
    int wl = width - 1;
    int index1, index2, index3, index4;
    for (int i = 0; i < hl; ++i) {
	for (int j = 0; j < wl; ++j) {
	    index1 = j + i * width;
	    index2 = j + 1 + i * width;
	    index3 = j + 1 + (i + 1) * width;
	    index4 = j + (i + 1) * width;
	    
	    if (isValueWithIn(vertices[index1].y, vertices[index2].y)) 
		getContourPoint(index1, index2, vertices, width, top);
	    if (isValueWithIn(vertices[index2].y, vertices[index3].y)) 
		getContourPoint(index2, index3, vertices, width, top);
	    if (isValueWithIn(vertices[index3].y, vertices[index1].y)) 
		getContourPoint(index3, index1, vertices, width, top);
	    
	    if (isValueWithIn(vertices[index1].y, vertices[index3].y)) 
		getContourPoint(index1, index3, vertices, width, top);
	    if (isValueWithIn(vertices[index3].y, vertices[index4].y)) 
		getContourPoint(index3, index4, vertices, width, top);
	    if (isValueWithIn(vertices[index4].y, vertices[index1].y)) 
		getContourPoint(index4, index1, vertices, width, top);
	}
    }
    return _points.size();
}


int 
ContourLineFilter::ContourLine::createLine(int width, int height, 
					   Vector4f* vertices, bool top)
{
    _points.clear();

    int hl = height - 1;
    int wl = width - 1;
    int index1, index2, index3, index4;
    for (int i = 0; i < hl; ++i) {
	for (int j = 0; j < wl; ++j) {
	    index1 = j + i * width;
	    index2 = j + 1 + i * width;
	    index3 = j + 1 + (i + 1) * width;
	    index4 = j + (i + 1) * width;
	    
	    if (isValueWithIn(vertices[index1].y, vertices[index2].y)) 
		getContourPoint(index1, index2, vertices, width, top);
	    if (isValueWithIn(vertices[index2].y, vertices[index3].y)) 
		getContourPoint(index2, index3, vertices, width, top);
	    if (isValueWithIn(vertices[index3].y, vertices[index1].y)) 
		getContourPoint(index3, index1, vertices, width, top);
	    
	    if (isValueWithIn(vertices[index1].y, vertices[index3].y)) 
		getContourPoint(index1, index3, vertices, width, top);
	    if (isValueWithIn(vertices[index3].y, vertices[index4].y)) 
		getContourPoint(index3, index4, vertices, width, top);
	    if (isValueWithIn(vertices[index4].y, vertices[index1].y)) 
		getContourPoint(index4, index1, vertices, width, top);
	}
    }
    
    return _points.size();
}


void 
ContourLineFilter::ContourLine::getContourPoint(int vertexIndex1, 
	int vertexIndex2, Vector3f* vertices, int width, bool top)
{
    float diff = vertices[vertexIndex2].y - vertices[vertexIndex1].y;
    float t = 0.0;
    if (diff != 0) {
	    t = (_value - vertices[vertexIndex1].y) / diff; 
    }

    Vector3f p;
    p.x = vertices[vertexIndex1].x + t * 
	(vertices[vertexIndex2].x - vertices[vertexIndex1].x);

    if (top)
    {
        p.y = 1.0f;
    }
    else
    {
    p.y = vertices[vertexIndex1].y + t * 
	(vertices[vertexIndex2].y - vertices[vertexIndex1].y);
    }

    p.z = vertices[vertexIndex1].z + t * 
	(vertices[vertexIndex2].z - vertices[vertexIndex1].z);
    _points.push_back(p);
}

void 
ContourLineFilter::ContourLine::getContourPoint(int vertexIndex1, 
	int vertexIndex2, Vector4f* vertices, int width, bool top)
{
    float diff = vertices[vertexIndex2].y - vertices[vertexIndex1].y;
    float t = 0.0;
    if (diff != 0) {
	t = (_value - vertices[vertexIndex1].y) / diff; 
    }

    Vector3f p;
    p.x = vertices[vertexIndex1].x + 
	t * (vertices[vertexIndex2].x - vertices[vertexIndex1].x);
    if (top)
    {
        p.y = 1.0f;
    }
    else
    {
        p.y = vertices[vertexIndex1].y + 
	    t * (vertices[vertexIndex2].y - vertices[vertexIndex1].y);
    }

    p.z = vertices[vertexIndex1].z + 
	t * (vertices[vertexIndex2].z - vertices[vertexIndex1].z);
    _points.push_back(p);
}


void 
ContourLineFilter::setColorMap(Vector3fArray* colorMap)
{
    if (colorMap == _colorMap) {
	return;
    }
    if (colorMap && _colorMap) {
	if (colorMap->size() != _colorMap->size()) {
	    _colorMap->resize(_colorMap->size());
	}
	_colorMap->assign(colorMap->begin(), colorMap->end());
    } else {
	delete _colorMap;
	
	if (colorMap && colorMap->size()) {	
	    _colorMap = new Vector3fArray(colorMap->size());
	    _colorMap->assign(colorMap->begin(), colorMap->end());
	} else {
	    _colorMap = 0;
	}
    }
}
