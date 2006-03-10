#ifndef __GRID1D_H__
#define __GRID1D_H__

//
// class for field values
//

#include "grid1d.h"

typedef double DataValType;

class RpField : public RpGrid1d {
public:
	// constructors
	RpField() { };
	RpField(int size);
	RpField(const char* name, int size=0);

	// set mesh link
	void setMesh(const char* meshId);

	// return number of bytes needed for serialization
	virtual int numBytes(); 

	// serialize data 
	// returns pointer to buffer 
	// number of bytes (nbytes) set
	//
	virtual char * serialize(int& nbytes);
	virtual RP_ERROR doSerialize(char * buf, int nbytes);

	// deserialize data
	virtual RP_ERROR deserialize(const char* buf);
	virtual RP_ERROR doDeserialize(const char* buf);

	virtual void xmlString(std::string& str);
	virtual void print();

	// destructor
	virtual ~RpField() { };

private:
	std::string m_meshName; // mesh
};

#endif
