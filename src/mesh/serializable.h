#ifndef __SERIALIZABLE_H__
#define __SERIALIZABLE_H__

#include <vector>
#include <string>
#include "util.h"
#include "rp_types.h"
#include "byte_order.h"

#define HEADER_SIZE 16

//
// base class for serializable rappture objects
//
class RpSerializable {
public:
	// return object name (e.g., output.field(f1d))
	virtual const char* objectName() = 0;

	// set object name
	virtual void objectName(const char* name) = 0;

	// return object type (e.g., FIELD, MESH3D)
	virtual const char* objectType() = 0;

	// object marshalling
	virtual char* serialize(int& nbytes) = 0;
	virtual RP_ERROR doSerialize(const char* buf, int n) = 0;

	// object unmarshalling
	virtual RP_ERROR deserialize(const char* buf) = 0;

	virtual int size() = 0;
	virtual int numBytes() = 0;

	virtual ~RpSerializable() { };

	// factory methods
	//virtual RpSerializable* create(const char* objectType);
	
	static void readHeader(const char* buf, string& ver, int& nbytes);
	static void writeHeader(char* buf, const char* ver, int nbytes);
	static void readObjectName(const char* buf, string& name);
	static void writeObjectName(char* buf, string& name);
	static void readArrayDouble(const char* buf, vector<double>& data, int& npts);
	static void writeArrayDouble(char* buf, vector<double>& data, int npts);

};

#endif
