#ifndef __GRID2D_H__
#define __GRID2D_H__

//
// class for 2D grid (x1 y1 x2 y2 ...)
//

#include <vector>
#include "util.h"

typedef double DataValType;

class RpGrid2d {
public:
	// constructors
	RpGrid2d();
	RpGrid2d(int npoints);
	RpGrid2d(DataValType* x, DataValType* y, int npoints);
	RpGrid2d(DataValType* val, int npoints);
	//RpGrid2d(DataValType** xy, int npoints);
	RpGrid2d(const char* buf); // instantiate with byte stream

	// return number of points in grid
	int numPoints() { return m_data.size() / 2; }
	DataValType* data(); // access data array

	// change the size of the grid after grid is constructed
	void resize(int npoints) { m_data.resize(npoints*2); }

	void addPoint(DataValType xval, DataValType yval);
	void addPoint(DataValType* val, int npoints);

	// serialize data 
        char * serialize(RP_ENCODE_ALG eflag=RP_NO_ENCODING,
                         RP_COMPRESSION cflag=RP_NO_COMPRESSION);

	int deserialize(const char* buf);

	// destructor
	virtual ~RpGrid2d() { };

	// TODO
	//virtual int xmlPut() { };
	//virtual int xmlGet() { };

private:
	// store points as an array of x1 y1 x2 y2 ...
	vector<DataValType> m_data; 
};

#endif
