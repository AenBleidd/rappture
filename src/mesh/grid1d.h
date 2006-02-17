#ifndef __GRID1D_H__
#define __GRID1D_H__

//
// class for 1D grid
//

#include <vector>
#include "util.h"

class RpGrid1d {
public:
	// constructors
	RpGrid1d();
	RpGrid1d(int size);
	RpGrid1d(double* data, int size); // makes a copy of data
	RpGrid1d(const char* buf); // instantiate with byte stream

	void addPoint(double val);

	// return number of points in grid
	int numPoints() { return m_data.size(); }

	// change the size of the grid after grid is constructed
	void resize(int npoints) { m_data.resize(npoints); }

	// serialize data 
	char * serialize(RP_ENCODE_ALG eflag=RP_NO_ENCODING, 
			 RP_COMPRESSION cflag=RP_NO_COMPRESSION);
	int deserialize(const char* buf);

	// destructor
	virtual ~RpGrid1d() { };

	// TODO
	//virtual int xmlPut() { };
	//virtual int xmlGet() { };

private:
	vector<double> m_data; // array of doubles
};

#endif
