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
	RpGrid1d(const char* objectName, int size=0);
	RpGrid1d(DataValType* data, int size); // makes a copy of data

	void objectName(const char* str);
	const char* objectName();
	const char* objectType();

	// return number of bytes needed for serialization
	int numBytes(); 

	// return number of points in grid
	int size() { return numPoints(); };


	// add all points to grid
	RP_ERROR addAllPoints(DataValType* val, int nitems);

	// add one point to grid
	void addPoint(DataValType val);

	// return number of points in grid

	// max num points that can be stored
	int capacity() { return m_data.capacity(); }


	// change the size of the grid after grid is constructed
	void resize(int npoints) { m_data.resize(npoints); }

	// return pointer to array of points
	DataValType* getData();

	// return copy of data array - caller must free memory when
	// not done
	DataValType* getDataCopy();

	// serialize data 
	// returns pointer to buffer 
	// number of bytes (nbytes) set
	//
	char * serialize(int& nbytes);
	RP_ERROR doSerialize(char * buf, int nbytes);

	// deserialize data
	RP_ERROR deserialize(const char* buf);
	RP_ERROR doDeserialize(const char* buf);

	void xmlString(std::string& str);
	void print();

	// destructor
	virtual ~RpGrid1d() { };

protected:
	int numPoints() { return m_data.size(); };

private:
	std::string m_name; // object name
	vector<DataValType> m_data; // array of doubles
};

#endif
