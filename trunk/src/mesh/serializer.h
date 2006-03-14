#ifndef __RPSERIALIZER_H__
#define __RPSERIALIZER_H__

#include <vector>
#include <map>
#include <string>
#include "serializable.h"
#include "field.h"
#include "grid2d.h"
#include "util.h"

//
// class managing serializable objects
//
class RpSerializer {
public:

	RpSerializer() { m_buf = 0; };

	// remove all objects, reset ref counts to zero.
	// delete buffer memory so the caller does not need to
	void clear();

	void addObject(RpSerializable* obj);
	void addObject(const char* key, RpSerializable* obj);

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

	struct rpStrCmp {
		bool operator()( const char* s1, const char* s2 ) const {
			return strcmp( s1, s2 ) < 0;
		}
	};

private:
	typedef map<const char*, RpSerializable*, rpStrCmp> typeObjMap;
	typedef map<const char*, int, rpStrCmp> typeRefCount;

	// archive of objects referenced by their names (e.g., output.field)
	typeObjMap m_objMap;

	// reference count for each object
	typeRefCount m_refCount;

	char* m_buf; // tmp buffer for serialization
};

#endif
