#ifndef __SERIALIZABLE_H__
#define __SERIALIZABLE_H__

#include "util.h"

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

	// object unmarshalling
	virtual RP_ERROR deserialize(const char* buf) = 0;

	virtual int size() = 0;
	virtual int numBytes() = 0;

	virtual ~RpSerializable() { };

	// factory methods
	//virtual RpSerializable* create(const char* objectType);
};

#endif
