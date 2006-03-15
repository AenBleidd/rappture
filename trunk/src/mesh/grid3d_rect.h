#ifndef __GRID3D_RECT_H__
#define __GRID3D_RECT_H__

//
// class for rectilinear 3D grid 
//  X1 X2 X3 ... Xn
//  Y1 Y2 Y3 ... Ym
//  Z1 Z2 Z3 ... Zk
//  a total of n*m*k points
//

#include "grid3d.h"


class RpGrid3dRect : public RpGrid3d {
public:
	// constructors
	RpGrid3dRect() { };
	RpGrid3dRect(const char* name) { objectName(name); };

	// return number of points in grid
	int numPoints() { 
		return m_xpts.size() * m_ypts.size() * m_zpts.size(); 
	};

	virtual int numBytes();

	// add one point to one axis
	void addPointX(DataValType x) { m_xpts.push_back(x); };
	void addPointY(DataValType y) { m_ypts.push_back(y); };
	void addPointZ(DataValType z) { m_zpts.push_back(z); };

	RP_ERROR addAllPointsX(DataValType* x, int xdim);
	RP_ERROR addAllPointsY(DataValType* y, int ydim);
	RP_ERROR addAllPointsZ(DataValType* z, int zdim);

	// take product of x, y and z arrays and expand data into m_data
	void expandData();

	// add all points to grid at once
	RP_ERROR addAllPoints(DataValType* xpts, int xdim,
			      DataValType* ypts, int ydim,
			      DataValType* zpts, int zdim);

	//virtual const char* objectType();
	//virtual void xmlString(std::string& str);
	//virtual void print();

	// serialize data 
        virtual char * serialize(int& nbytes);
	virtual RP_ERROR doSerialize(char* buf, int nbytes);

	//virtual RP_ERROR deserialize(const char* buf);

	// destructor
	virtual ~RpGrid3dRect() { };

private:
	vector<DataValType> m_xpts;
	vector<DataValType> m_ypts; 
	vector<DataValType> m_zpts; 
};

#endif
