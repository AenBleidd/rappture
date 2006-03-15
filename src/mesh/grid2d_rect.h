#ifndef __GRID2D_RECT_H__
#define __GRID2D_RECT_H__

//
// class for rectilinear 2D grid 
//  X1 X2 X3 ... Xn
//  Y1 Y2 Y3 ... Ym
//  a total of n*m points
//

#include "grid2d.h"
#include "util.h"

typedef double DataValType;

class RpGrid2dRect : public RpGrid2d {
public:
	// constructors
	RpGrid2dRect() { };
	RpGrid2dRect(const char* name) { objectName(name); };

	// return number of points in grid
	int numPoints() { return m_xpts.size() * m_ypts.size(); };

	virtual int numBytes();

	// add one point to one axis
	void addPointX(DataValType x) { m_xpts.push_back(x); };
	void addPointY(DataValType y) { m_ypts.push_back(y); };

	RP_ERROR addAllPointsX(DataValType* x, int xdim);
	RP_ERROR addAllPointsY(DataValType* y, int ydim);

	// take product of x and y arrays and expand data into m_data
	// should be called after adding all x's and y's 
	void expandData();

	// add all points to grid at once
	RP_ERROR addAllPoints(DataValType* xpts, int xdim,
			      DataValType* ypts, int ydim);

	//virtual const char* objectType();
	//virtual void xmlString(std::string& str);
	//virtual void print();

	// serialize data 
        virtual char * serialize(int& nbytes);
	virtual RP_ERROR doSerialize(char* buf, int nbytes);

	//virtual RP_ERROR deserialize(const char* buf);

	// destructor
	virtual ~RpGrid2dRect() { };

private:
	vector<DataValType> m_xpts;
	vector<DataValType> m_ypts; 

	//void copyArray(DataValType* val, int dim, vector<DataValType>& vec);
};

#endif
