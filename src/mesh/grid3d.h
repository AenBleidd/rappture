#ifndef __GRID3D_H__
#define __GRID3D_H__

//
// class for 3D grid (x1 y1 z1 x2 y2 z2 ...)
//

#include <vector>
#include "grid1d.h"
#include "util.h"

typedef double DataValType;

class RpGrid3d : public RpGrid1d {
public:
	// constructors
	RpGrid3d();
	RpGrid3d(const char* name);
	RpGrid3d(int npoints);
	RpGrid3d(const char* name, int npts);

	// constructor - input as an array {x, y, z, x, y, z,...}
	RpGrid3d(DataValType* val, int npoints);

	// constructor - input as array of char pointers:
	// 	{ {x, y, z}, {x, y, z}, ... }
	RpGrid3d(DataValType* val[], int npoints);

	// constructor - uniform 3d grid, data expanded inclusive of 
	// 		origin and max
	RpGrid3d(DataValType x_min, DataValType x_max, DataValType x_delta,
                 DataValType y_min, DataValType y_max, DataValType y_delta,
                 DataValType z_min, DataValType z_max, DataValType z_delta);

	// constructor - 3d rectilinear grid
	// 		data expanded, row major
	RpGrid3d(DataValType* x, int xdim, DataValType* y, int ydim,
			DataValType* z, int zdim);

	// return number of points in grid
	virtual int numPoints() { return m_data.size()/3; };

	// add one point to grid
	void addPoint(DataValType x, DataValType y, DataValType z);

	// add all points to grid at once
	RP_ERROR addAllPoints(DataValType* points, int npoints);

	RP_ERROR setUniformGrid(
			DataValType xmin, DataValType xmax, DataValType xdelta,
                        DataValType ymin, DataValType ymax, DataValType ydelta,
                        DataValType zmin, DataValType zmax, DataValType zdelta);

	// add points of a rectilinear grid
	RP_ERROR setRectGrid(DataValType* x, int xdim, 
			     DataValType* y, int ydim,
			     DataValType* z, int zdim);

	// get pointer to data {x y z x y z...}
	DataValType* getData();

	// get point x y at index
	RP_ERROR getData(DataValType& x, DataValType& y, DataValType& z, 
			int index);

	virtual const char* objectType();
	virtual void xmlString(std::string& str);
	virtual void print();

	// serialize data 
        //virtual char * serialize(int& nbytes);
	virtual RP_ERROR doSerialize(char* buf, int nbytes);

	virtual RP_ERROR deserialize(const char* buf);

	// destructor
	virtual ~RpGrid3d() { };
};

#endif
