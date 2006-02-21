#ifndef __RP_NODE2D_H__
#define __RP_NODE2D_H__

#include <iostream>
#include <vector>
#include "util.h"

//
// define a 2-d node (x, y)
//
class RpNode2d {
public:
        // constructors
        RpNode2d() { };

        RpNode2d(int id): m_id(id)
	{ };

        RpNode2d(int id, int x, int y):
		m_id(id),
		m_x(x),
		m_y(y)
	{ };

	// assign value to coordinate[index]
	void set(int x, int y) { m_x = x; m_y = y; };
	void set(int* val) { m_x = val[0]; m_y = val[1]; };

	// get node coordinates
	void get(int& xval, int& yval) 
	{ xval = m_x; yval = m_y; };

	void get(int * val) { val[0] = m_x; val[1] = m_y; };

	int id() { return m_id; };
	void id(int id) { m_id = id; };

	char* serialize();
	RP_ERROR serialize(char* buf, int buflen);
	RP_ERROR deserialize(const char* buf);

        void print();

        virtual ~RpNode2d() { };

private:
	int m_id;
	int m_x;
	int m_y;
};

#endif
