#ifndef __RP_FIELD_H__
#define __RP_FIELD_H__

//
// class for field values
//

#include "grid1d.h"
#include "util.h"
#include "rp_types.h"

class RpField : public RpGrid1d {
public:
	// constructors
	RpField() { };

	RpField(int size) : RpGrid1d(size) { };

	RpField(const char* name, int size=0) : RpGrid1d(name,size) { };

	// set mesh link
	void setMesh(const char* meshId);

	// return number of bytes needed for serialization
	virtual int numBytes(); 

	// return object type as a char string
	virtual const char* objectType();

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
};

#endif
