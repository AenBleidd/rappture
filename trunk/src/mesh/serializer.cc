#include "serializer.h"

//
// add an serializable object to the Serializer
//
void 
Serializer::addObject(RpSerializable* obj)
{
	// add object pointer to object map
	m_objMap[obj->objectName()] = obj; 

	// add object entry to reference count table
	m_refCount[obj->objectName()] = 0; 
}

// 
// Input:
// 	a byte stream of a serialized object
// 		RV-<object-type>-A<numBytes>.....
//
void 
Serializer::addObject(const char* buf)
{
	// add object pointer to object map
	m_objMap[obj->objectName()] = obj; 

	// add object entry to reference count table
	m_refCount[obj->objectName()] = 0; 
}

//
// Remove an object from Serializer
// Input:
// 	name of rp object (e.g., "output.mesh(m3d)")
//
void 
Serializer::deleteObject(const char* name);
{
	typeObjMap::iterator iter = m_objMap.find(name);
	if (iter == m_objMap.end())
		// does not exist
		return;

	Serializable* obj_ptr = (*iter).second;

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
Serializer::deleteObject(Serializable* obj)
{
	typeObjMap::iterator iter = m_objMap.find(name);
	if (iter == m_objMap.end())
		// does not exist
		return;

	Serializable* obj_ptr = (*iter).second;

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
Serializer::deleteAllObjects()
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
Serializable* 
RpSerializer::getObject(const char* objectName)
{
	typeObjMap::iterator iter m_objMap.find(objectName);
	
	if (iter != m_objMap.end()) {
		// object found, increment ref count
		++m_refCount[objectName];

		// return pointer to object
		return (*iter).second;
	}
	else // not found
		return NULL;
}

// marshalling
char* 
RpSerializer::serialize()
{
	// calculate total number of bytes by adding up all the objects
	int nbytes = 0;	

	typeObjMap::iterator iter;
	for (iter = m_objMap.begin(); iter != m_objMap.end(); iter++)
		nbytes += ( (*iter).second)->numBytes();

	// add bytes necessary for encoding and compression and
	// total number of bytes
	int headerSize = sizeof(int) + 8;

	nbytes += headerSize;
	
	// allocate memory
	char* buf = new char(nbytes);
	if (buf == NULL) {
		printf("RpSerializer::serialize(): failed to allocate mem\n");
		return buf;
	}

	// call each object to serialize itself
	char* ptr = buf + headerSize;
	for (iter = m_objMap.begin(); iter != m_objMap.end(); iter++) {
		Serializable* obj = (*iter).second;
		obj->serialize(ptr); // object serialization
		ptr += obj->numBytes(); // advance buffer pointer
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
	// 
}


