#ifndef __GRID1D_H__
#define __GRID1D_H__

//
// class for 1D grid
//

#include <vector>
#include <string>
#include "serializable.h"
#include "byte_order.h"
#include "util.h"

typedef double DataValType;

class RpGrid1d : public RpSerializable {
public:
	// constructors
	RpGrid1d();
	RpGrid1d(int size);
	RpGrid1d(const char* objectName, int size);
	RpGrid1d(DataValType* data, int size); // makes a copy of data

	// constructor for a regular grid with start point and delta
	RpGrid1d(DataValType startPoint, DataValType delta, int npts);

	virtual void objectName(const char* str);
	virtual const char* objectName();
	virtual const char* objectType();

	// return number of bytes needed for serialization
	virtual int numBytes(); 

	// return number of points in grid
	virtual int size() { return numPoints(); };

	// add all points to grid
	virtual RP_ERROR addAllPoints(DataValType* val, int nitems);

	// add one point to grid
	virtual void addPoint(DataValType val);

	// return number of points in grid

	// max num points that can be stored
	virtual int capacity() { return m_data.capacity(); }

	// change the size of the grid after grid is constructed
	virtual void resize(int npoints) { m_data.resize(npoints); }

	// return pointer to array of points
	virtual DataValType* getData();

	// return copy of data array - caller must free memory when
	// not done
	virtual DataValType* getDataCopy();

	// serialize data 
	// returns pointer to buffer 
	// number of bytes (nbytes) set
	//
	virtual char * serialize(int& nbytes);
	virtual RP_ERROR doSerialize(char * buf, int nbytes);

	// deserialize data
	virtual RP_ERROR deserialize(const char* buf);
	virtual RP_ERROR doDeserialize(const char* buf);

	virtual void xmlString(std::string& str);
	virtual void print();

	// destructor
	virtual ~RpGrid1d() { };

protected:
	int numPoints() { return m_data.size(); };

	std::string m_name; // object name
	vector<DataValType> m_data; // array of doubles

};

#endif
