#ifndef _CONTOURFILTER_H_
#define _CONTOURFILTER_H_

#include "Vector3.h"
#include "Vector4.h"
#include "TypeDefs.h"
#include <R2/graphics/R2Geometry.h>
#include <list>

class ContourLineFilter {
public :
	class ContourLine {
	public :
		float _value;
		std::list<Vector3> _points;
	public :
		ContourLine(float value);

	private :
		bool isValueWithIn(float Val1, float Val2); 
		void getContourPoint(int vertexIndex1, int vertexIndex2, Vector3* vertices, int width);
		void getContourPoint(int vertexIndex1, int vertexIndex2, Vector4* vertices, int width);
	public :  
		/**
		 * @brief
		 * @ret Returns the number of points
		 */
		int createLine(int width, int height, Vector3* vertices);
		int createLine(int width, int height, Vector4* vertices);
	};

	typedef std::list<ContourLine*> ContourLineList;

private :
	ContourLineList _lines;
	Vector3Array* _colorMap;
public :
	ContourLineFilter();

private :
	void clear();
	
public :
	R2Geometry* create(float min, float max, int linecount, Vector3* vertices, int width, int height);
	R2Geometry* create(float min, float max, int linecount, Vector4* vertices, int width, int height);
	void setColorMap(Vector3Array* colorMap);
};

#endif 
