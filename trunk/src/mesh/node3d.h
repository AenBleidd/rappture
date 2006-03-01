#ifndef __RP_NODE3D_H__
#define __RP_NODE3D_H__

#include <iostream>
#include <vector>
#include "util.h"

//
// define a 3-d node (x, y, z)
//
class RpNode3d {
public:
        // constructors
        RpNode3d() { };

        RpNode3d(int id): m_id(id)
	{ };

        RpNode3d(int id, int x, int y, int z):
		m_id(id),
		m_x(x),
		m_y(y),
		m_z(z)
	{ };

	void operator=(const int* val)
		{ m_x = val[0]; m_y = val[1]; m_z = val[2]; };
	
	// assign value to coordinate[index]
	void set(int x, int y, int z) 
		{ m_x = x; m_y = y; m_z =z; };
	void set(int* val) 
		{ m_x = val[0]; m_y = val[1]; m_z = val[2]; };

	// get node coordinates
	void get(int& xval, int& yval, int& zval) 
	{ xval = m_x; yval = m_y; zval = m_z; };
	void get(int* xval, int* yval, int* zval) 
	{ *xval = m_x; *yval = m_y; *zval = m_z; };

	void get(int * val) { val[0] = m_x; val[1] = m_y; val[2] = m_z; };

	int id() { return m_id; };
	void id(int id) { m_id = id; };

	char* serialize();
	RP_ERROR serialize(char* buf);
	RP_ERROR deserialize(const char* buf);

	void xmlString(std::string& str);
        void print();

        void erase() {
		m_id = 0;
		m_x = m_y = m_z = 0;
	};

        virtual ~RpNode3d() { };

private:
	int m_id;
	int m_x;
	int m_y;
	int m_z;
};

#endif
