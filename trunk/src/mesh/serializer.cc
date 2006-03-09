#include "serializer.h"
#include "rp_types.h"

//
// add an serializable object to the Serializer
//
void 
RpSerializer::addObject(RpSerializable* obj)
{
	const char* key = obj->objectName();
	// add object pointer to object map
	m_objMap[key] = obj; 

	// add object entry to reference count table
	m_refCount[key] = 0; 
#ifdef DEBUG
	printf("RpSerializer::addObject: obj name=%s, ptr=%x\n", key, (unsigned int)obj);
	printf("RpSerializer::addObject: map size %d\n", m_objMap.size());
#endif
}

// 
// Input:
// 	a byte stream of a serialized object
// 		RV-A-<object-type><numBytes>.....
//
void 
RpSerializer::addObject(const char* buf)
{
	// todo 
	
}

//
// Remove an object from Serializer
// Input:
// 	name of rp object (e.g., "output.mesh(m3d)")
//
void 
RpSerializer::deleteObject(const char* name)
{
	typeObjMap::iterator iter = m_objMap.find(name);
	if (iter == m_objMap.end())
		// does not exist
		return;

	RpSerializable* obj_ptr = (*iter).second;

	// decrement object's reference count
	// if reference count is zero, free memory and remove entry in obj map
	if ( (--(m_refCount[name])) == 0) {
		delete obj_ptr; // free obj memory
		m_objMap.erase(name); // erase entry in object map
		m_refCount.erase(name); // erase entry in object map
	}

	return;
}

void 
RpSerializer::deleteObject(RpSerializable* obj)
{
	const char* name = obj->objectName();
	typeObjMap::iterator iter = m_objMap.find(name);

	if (iter == m_objMap.end())
		// does not exist
		return;

	RpSerializable* obj_ptr = (*iter).second;

	// decrement object's reference count
	// if reference count is zero, free memory and remove entry in obj map
	if ( (--(m_refCount[name])) == 0) {
		delete obj_ptr; // free obj memory
		m_objMap.erase(name); // erase entry in object map
		m_refCount.erase(name); // erase entry in object map
	}

	return;
}

void 
RpSerializer::deleteAllObjects()
{
	typeObjMap::iterator iter;

	// free memory for all objects
	for (iter = m_objMap.begin(); iter != m_objMap.end(); iter++)
		delete (*iter).second;

	m_objMap.clear();
	m_refCount.clear();
	m_numObjects = 0;
}

void
RpSerializer::clear()
{
	deleteAllObjects();
}

// retrieve object
RpSerializable* 
RpSerializer::getObject(const char* objectName)
{
	typeObjMap::iterator iter = m_objMap.find(objectName);
	
	if (iter != m_objMap.end()) {
#ifdef DEBUG
		printf("RpSerializer::getObject: found %s %x\n", objectName,
				(unsigned)( (*iter).second ));
#endif
		// object found, increment ref count
		++(m_refCount[objectName]);

		// return pointer to object
		return (*iter).second;
	}
	else {// not found
#ifdef DEBUG
		printf("RpSerializer::getObject: obj %s not found\n", objectName);
#endif
		return NULL;
	}
}

// marshalling
// The data format for the serialized byte stream:
// 4 bytes: total number bytes (including these 4 bytes)
// 2 bytes: encoding flag
// 2 bytes: compression flag
// Each object in the serializer is written out to a byte stream in its
// format: 
// 	RV-A-GRID1D
// 	total num bytes
// 	number of chars in object name
// 	object name (e.g., output.grid(g1d) )
//	num points
//	x1 x2 ...
// 
char* 
RpSerializer::serialize()
{
	// calculate total number of bytes by adding up all the objects
	int nbytes = numBytes();	

	// add bytes necessary for encoding and compression and
	// total number of bytes:

	nbytes += headerSize();

#ifdef DEBUG
	printf("RpSerializer::serialize: bytes=%d\n", nbytes);
#endif
	
	// allocate memory
	char* buf = new char[nbytes];
	if (buf == NULL) {
		printf("RpSerializer::serialize(): failed to allocate mem\n");
		return buf;
	}

#ifdef DEBUG
	printf("RpSerializer::serialize: begin buf=%x\n", (unsigned)buf);
#endif

	writeHeader(buf, nbytes, "NO", "NO");

	// call each object to serialize itself
	char* ptr = buf + headerSize();

#ifdef DEBUG
	printf("RpSerializer::serialize: obj buf=%x\n", (unsigned)ptr);
#endif

	typeObjMap::iterator iter;
	int nb;
	for (iter = m_objMap.begin(); iter != m_objMap.end(); iter++) {
		RpSerializable* obj = (*iter).second;
		nb = obj->numBytes();
		obj->doSerialize(ptr, nb); // object serialization
		ptr += nb; // advance buffer pointer
	}

	// return pointer to buffer
	return buf;
}

//
// unmarshalling 
//
// Input:
// 	buf - pointer to byte stream buffer, including the beginning bytes
// 	      for total number of bytes, compression and encoding indicators.
// Side effects:
// 	objects are created in m_objMap
// 	reference count for each object is set to 0
void 
RpSerializer::deserialize(const char* buf)
{
	// todo
	
}

// 
// returns total number of bytes from the objects
//
int RpSerializer::numBytesObjects()
{
	int nbytes = 0;	
	typeObjMap::iterator iter;

	for (iter = m_objMap.begin(); iter != m_objMap.end(); iter++)
		nbytes += ( (*iter).second)->numBytes();

	return nbytes;
}

void RpSerializer::readHeader(const char* buf, int& nbytes, 
		std::string& encodeFlag, std::string& compressFlag)
{
	char* ptr = (char*)buf;

	// read number of bytes
	readInt(ptr, nbytes);
	ptr += sizeof(int);

	// read encoding flag (2 bytes)
	readString(ptr, encodeFlag);
	readString(ptr+2, encodeFlag);

#ifdef DBEUG
	printf("Serializer::readHeader: nbytes=%d ", nbytes);
	printf("encode: %s ", encodeFlag.c_str());
	printf("compress: %s\n", compressFlag.c_str());
#endif

}

// 
//
void RpSerializer::writeHeader(char* buf, int nbytes, const char* eflag, const char* cflag)
{
	char* ptr = (char*)buf;

	// read number of bytes
	writeInt(ptr, nbytes);
	ptr += sizeof(int);

	// read encoding flag (2 bytes)
	writeString(ptr, eflag, 2);
	writeString(ptr+2, cflag, 2);
}

void RpSerializer::print()
{
	typeObjMap::iterator iter;
	for (iter = m_objMap.begin(); iter != m_objMap.end(); iter++)
		printf("objMap: %s \t %x\n", (*iter).first, (unsigned)(*iter).second);

	typeRefCount::iterator it;
	for (it = m_refCount.begin(); it != m_refCount.end(); it++)
		printf("count: %s \t %d\n", (*it).first, (*it).second);
};
