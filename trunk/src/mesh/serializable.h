#ifndef __SERIALIZABLE_H__
#define __SERIALIZABLE_H__

#include <vector>
#include <string>
#include "util.h"
#include "rp_types.h"
#include "byte_order.h"

//#define HEADER_SIZE 16

//
// base class for serializable rappture objects
//
class RpSerializable {
public:
	friend class RpSerializer;

	// return object name (e.g., output.field(f1d))
	virtual const char* objectName() = 0;

	// set object name
	virtual void objectName(const char* name) = 0;

	// set and get object type (e.g., FIELD, MESH3D)
	virtual const char* objectType() = 0;

	// object marshalling
	virtual char* serialize(int& nbytes) = 0;
	virtual RP_ERROR doSerialize(char* buf, int n) = 0;

	// object unmarshalling
	virtual RP_ERROR deserialize(const char* buf) = 0;

	virtual int size() = 0;
	virtual int numBytes() = 0;

	virtual void print() = 0;
	virtual void xmlString(std::string& str) = 0;

	virtual ~RpSerializable() { };

	// factory methods
	//virtual RpSerializable* create(const char* objectType);
	
	/*
	virtual void readHeader(const char* buf, string& ver, int& nbytes);
	virtual void writeHeader(char* buf, const char* ver, int nbytes);
	virtual void readString(const char* buf, string& str);
	virtual void writeString(char* buf, string& str);
	virtual void readInt(const char* buf, int& val);
	virtual void writeInt(char* buf, int val);
	virtual void readArrayInt(const char* buf, vector<int>& data, int& npts);
	virtual void writeArrayInt(char* buf, vector<int>& data, int npts);
	virtual void readArrayDouble(const char* buf, vector<double>& data, int& npts);
	virtual void writeArrayDouble(char* buf, vector<double>& data, int npts);
	*/
};

#endif
