#ifndef __GRID2D_H__
#define __GRID2D_H__

//
// class for 2D grid (x1 y1 x2 y2 ...)
//

#include <vector>
#include "util.h"

class RpGrid2d {
public:
	// constructors
	RpGrid2d();
	RpGrid2d(int npoints);
	RpGrid2d(double* xval, double* yval, int npoints);
	RpGrid2d(double** data, int npoints);
	RpGrid2d(const char* buf); // instantiate with byte stream

	// return number of points in grid
	int numPoints() { return m_data.size() / 2; }

	// change the size of the grid after grid is constructed
	void resize(int npoints) { m_data.resize(npoints*2); }

	void addPoint(double xval, double yval);

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
	vector<double> m_data; // array of doubles
};

#endif
