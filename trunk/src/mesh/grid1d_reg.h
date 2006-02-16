#ifndef __GRID1D_REG_H__
#define __GRID1D_REG_H__

//
// class for a regular 1D grid, 
// defined by  a starting point and delta increment.
// 
//

#include <vector>
#include "rp_defs.h"

class RpGrid1d_reg {
public:
	// constructors
	RpGrid1d_reg() :
		_npoints(0),
		_start(0.0),
		_delta(0.0)
	{ };

	// does not expand to array
	RpGrid1d_reg(int npts, double delta, double st) :
		_npoints(npts),
		_start(st),
		_delta(delta)
	{ };

	RpGrid1d_reg(int npts, double delta):
		_npoints(npts),
		_start(0.0),
		_delta(delta)
	{ };

	// copy constructor
	//RpGrid1d_reg(const RpGrid1d_reg& c);

	double delta() { return _delta; };
	void delta(double d) { _delta = d; };
	double start() { return _start; };
	void start(double s) { _start = s; };
	int points() { return _npoints; };
	void points(int n) { _npoints = n; };

	// construct the object with byte stream containing number of points and delta value
	RpGrid1d_reg(const char* buf);

	// return number of points in grid
	int npoints() { return _npoints; }

	// serialize object 
	char * serialize();
	int deserialize(const char* buf);

	// expand to array
	double* data();

	// destructor
	virtual ~RpGrid1d_reg() { };

	// TODO
	//virtual int xmlPut() { };
	//virtual int xmlGet() { };

private:
	int _npoints;
	double _start;
	double _delta;
	vector<double> _data;
};

#endif
