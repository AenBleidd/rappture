#ifndef __RP_FIELD_H__
#define __RP_FIELD_H__

//
// Class for field object
//
// Example usage:
//	mesh = new RpGrid2d;
//	mesh->addAllPoints(x,y);
//	field = new RpField;
//	field->addValues(val);
// 
//

#include "grid1d.h"
#include "util.h"
#include "rp_types.h"

class RpField : public RpGrid1d {
public:
	// constructors
	RpField() { m_mesh = NULL; };

	RpField(int size) : RpGrid1d(size) { m_mesh = NULL; };

	RpField(const char* name, int size=0) : RpGrid1d(name,size)
		{ m_mesh = NULL; };

	// set name of mesh object used in field 
	void setMeshId(const char* meshId) { m_meshName.assign(meshId); };

	// set name of mesh object used in field 
	RP_ERROR setMeshObj(RpSerializable* meshPtr);

	// remove mesh - only erase name and remove ptr (not free memory as serializer manages the object memory)
	void removeMesh();

	const char* getMeshId() { return m_meshName.c_str(); };
	RpSerializable* getMeshObj() { return m_mesh; };

	// return number of bytes needed for serialization
	virtual int numBytes(); 

	// return object type as a char string
	virtual const char* objectType();

	virtual RP_ERROR addAllValues(double* val, int nitems) {
		return RpGrid1d::addAllPoints(val, nitems); 
	};

	virtual void addValue(double val) {
		RpGrid1d::addPoint(val); 
	};

	// serialize data 
	// returns pointer to buffer 
	// number of bytes (nbytes) set
	//
	virtual char * serialize(int& nbytes);
	virtual RP_ERROR doSerialize(char * buf, int nbytes);

	// deserialize data
	virtual RP_ERROR deserialize(const char* buf);
	virtual RP_ERROR doDeserialize(const char* buf);

	// remove all data
	virtual void clear();

	// serialize object content to an XML text string
	virtual void xmlString(std::string& str);

	// print content of object to stdout
	virtual void print();

	// assignment operator
	virtual RpField operator=(const RpField& f);


	// destructor
	virtual ~RpField() { };

private:
	std::string m_meshName; // mesh
	RpSerializable* m_mesh; // pointer to mesh object
};

#endif
