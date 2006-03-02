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

	// add all points to grid
	RP_ERROR addAllPoints(double* val, int nitems);

	// add one point to grid
	void addPoint(double val);

	// return number of points in grid
	int numPoints() { return m_data.size(); }

	// max num points that can be stored
	int capacity() { return m_data.capacity(); }

	// change the size of the grid after grid is constructed
	void resize(int npoints) { m_data.resize(npoints); }

	// serialize data 
	// If no input parameters are provided, byte stream will be binary
	// without compression.
	//
	char * serialize(int& nbytes, RP_ENCODE_ALG eflag=RP_NO_ENCODING, 
			 RP_COMPRESSION cflag=RP_NO_COMPRESSION);

	RP_ERROR serialize(char * buf, int nbytes,
			RP_ENCODE_ALG eflag=RP_NO_ENCODING, 
			RP_COMPRESSION cflag=RP_NO_COMPRESSION);

	RP_ERROR deserialize(const char* buf);

	void xmlString(std::string& str);
	void print();

	// destructor
	virtual ~RpGrid1d() { };

	// TODO
	//virtual int xmlPut() { };
	//virtual int xmlGet() { };

private:
	vector<double> m_data; // array of doubles
};

#endif
