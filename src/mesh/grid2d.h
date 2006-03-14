#ifndef __GRID2D_H__
#define __GRID2D_H__

//
// class for 2D grid (x1 y1 x2 y2 ...)
//

#include <vector>
#include "grid1d.h"
#include "util.h"

typedef double DataValType;

class RpGrid2d : public RpGrid1d {
public:
	// constructors
	RpGrid2d();
	RpGrid2d(int npoints);

	// constructor - input as 1d array of points {x, y, x, y ,...}
	RpGrid2d(DataValType* val, int npoints);

	// constructor - input as array of char pointers:
	// 	{ {x, y}, {x, y}, ... }
	RpGrid2d(DataValType* val[], int npoints);

	// constructor - uniform 2d grid, data expanded inclusive of 
	// 		origin and max
	RpGrid2d(DataValType x_min, DataValType x_max, DataValType x_delta,
                 DataValType y_min, DataValType y_max, DataValType y_delta);

	// constructor - 2d rectilinear grid
	// 		data expanded, row major
	RpGrid2d(DataValType* x, int xdim, DataValType* y, int ydim);

	// return number of points in grid
	virtual int numPoints() { return m_data.size()/2; };

	// add one point to grid
	void addPoint(DataValType x, DataValType y);

	// add all points to grid at once
	RP_ERROR addAllPoints(DataValType* points, int npoints);

	RP_ERROR setUniformGrid(DataValType xmin, DataValType xmax, 
			DataValType xdelta,
                        DataValType ymin, DataValType ymax, DataValType ydelta);

	// add points of a rectilinear grid
	RP_ERROR setRectGrid(DataValType* x, int xdim, 
			     DataValType* y, int ydim);

	// get point x y at index
	DataValType* getData();
	RP_ERROR getData(DataValType& x, DataValType& y, int index);

	virtual const char* objectType();
	virtual void xmlString(std::string& str);
	virtual void print();

	// serialize data 
        //virtual char * serialize(int& nbytes);
	virtual RP_ERROR doSerialize(char* buf, int nbytes);

	virtual RP_ERROR deserialize(const char* buf);

	// destructor
	virtual ~RpGrid2d() { };

protected:
	// store points as an array of x1 y1 x2 y2 ...
	//vector<DataValType> m_data; 
};

#endif
