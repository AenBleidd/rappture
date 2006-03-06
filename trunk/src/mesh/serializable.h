#ifndef __SERIALIZABLE_H__
#define __SERIALIZABLE_H__

//
// base class for serializable rappture objects
//
class RpSerializable {
public:
	// return object name (e.g., output.field(f1d))
	virtual const char* objectName() = 0;

	// return object type (e.g., FIELD, MESH3D)
	virtual const char* objectType() = 0;

	// object marshalling
	virtual void serialize() = 0;

	// object unmarshalling
	virtual void deserialize() = 0;

	virtual int size() = 0;
	virtual int numBytes() = 0;
};

#endif
