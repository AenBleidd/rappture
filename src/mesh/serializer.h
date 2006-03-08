#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include <vector>
#include <map>
#include "serializable.h"

//
// class managing serializable objects
//
class RpSerializer {
public:

	RpSerializer() { };

	// remove all objects, reset ref counts to zero.
	void clear();

	void addObject(RpSerializable* obj);
	void addObject(const char* buf);

	void deleteObject(RpSerializable* obj);
	void deleteObject(const char* objectName);
	void deleteAllObjects();

	// marshalling
	char* serialize();

	// unmarshalling - instantiate all objects
	void deserialize(const char* buf);

	// retrieve object
	RpSerializable* getObject(const char* objectName);

private:
	typedef map<const char*, RpSerializable*> typeObjMap;
	typedef map<const char*, int> typeRefCount;

	// archive of objects referenced by their names (e.g., output.field)
	typeObjMap m_objMap;

	// reference count for each object
	typeRefCount m_refCount;

	unsigned int m_numObjects; //? can get from map.size()
};

#endif
