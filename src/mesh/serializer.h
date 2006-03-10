#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include <vector>
#include <map>
#include <string>
#include "grid1d.h"
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
	//void addObject(const char* buf);

	void deleteObject(RpSerializable* obj);
	void deleteObject(const char* objectName);
	void deleteAllObjects();

	// marshalling
	char* serialize();

	// unmarshalling - instantiate all objects
	void deserialize(const char* buf);

	// retrieve object
	RpSerializable* getObject(const char* objectName);

	// returns total number of bytes of all objects in serializer
	int numBytesObjects();

	// returns total number of bytes including objects and header
	int numBytes() { return (numBytesObjects() + headerSize()); };

	// returns total number of bytes in header
	// header contains the following:
	// 	blob id (in the form of: RV-A-TYPENAME)
	// 	   where A is a version number represented by a char in A-Z
	// 	   where TYPENAME is a char string, e.g., FIELD, GRID1D.
	// 	2 bytes: encoding indicator (id encoding algorithm)
	// 	2 bytes: compression indicator (id compression algorithm)
	//
	int headerSize() { return 8; };

	void print();

protected:
	// read/write file header
	void readHeader(const char* buf, int& nbytes, 
			std::string& encodeFlag, std::string& compressFlag);
	void writeHeader(char* buf, int nbytes, 
			const char* eflag, const char* cflag);

	RpSerializable* createObject(std::string header, const char* buf);

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
