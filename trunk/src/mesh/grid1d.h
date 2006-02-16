#ifndef __GRID1D_H__
#define __GRID1D_H__

//
// class for 1D grid
//

#include <vector>
#include "rp_defs.h"

class RpGrid1d {
public:
	// constructors
	RpGrid1d();
	RpGrid1d(int size);
	RpGrid1d(double* data, int size); // makes a copy of data
	RpGrid1d(const char* buf); // instantiate with byte stream

	//int type() { return _type; }
	//void type(int i) { _type = i; }

	// return number of points in grid
	int size() { return _points.size(); }

	// change the size of the grid after grid is constructed
	void resize(int size) { _points.resize(size); }

	// serialize data 
	char * serialize();
	int deserialize(const char* buf);

	// destructor
	virtual ~RpGrid1d() { };

	// TODO
	//virtual int xmlPut() { };
	//virtual int xmlGet() { };

private:
	int _type;	// type of array elements: int, float, double, etc.
	vector<double> _points; // array of doubles
};

#endif
