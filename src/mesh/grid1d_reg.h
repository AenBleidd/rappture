#ifndef __GRID1D_REG_H__
#define __GRID1D_REG_H__

//
// class for a regular 1D grid, 
// defined by  a starting point and delta increment.
// 
//
#include <vector>
#include "util.h"

class RpGrid1d_reg {
public:
	// constructors
	RpGrid1d_reg() :
		m_npoints(0),
		m_start(0.0),
		m_delta(0.0)
	{ };

	// does not expand to array
	RpGrid1d_reg(int npts, double delta, double st) :
		m_npoints(npts),
		m_start(st),
		m_delta(delta)
	{ };

	RpGrid1d_reg(int npts, double delta):
		m_npoints(npts),
		m_start(0.0),
		m_delta(delta)
	{ };

	// copy constructor
	//RpGrid1d_reg(const RpGrid1d_reg& c);

	double delta() { return m_delta; };
	void delta(double d) { m_delta = d; };
	double start() { return m_start; };
	void start(double s) { m_start = s; };
	int points() { return m_npoints; };
	void points(int n) { m_npoints = n; };

	// construct the object with byte stream containing number of points and delta value
	RpGrid1d_reg(const char* buf);

	// return number of points in grid
	int npoints() { return m_npoints; }

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
	int m_npoints;
	double m_start;
	double m_delta;
	vector<double> m_data;
};

#endif
