//
// class for 1D grid
//

#include "serializable.h"
#include "rp_types.h"
#include "grid1d.h"


// constructor
RpGrid1d::RpGrid1d()
{
}

RpGrid1d::RpGrid1d(int size)
{
	m_data.reserve(size);
}

// constructor with object name
RpGrid1d::RpGrid1d(const char* name, int size)
{
	m_data.reserve(size);
	m_name = name;
}

/*
// construct a grid object from a byte stream
RpGrid1d::RpGrid1d(const char * buf)
{
#ifdef DEBUG
printf("RpGrid1d(char*)\n");
#endif
	deserialize(buf);
}
*/

//
// instantiate grid with a 1d array of doubles and number of items
//
RpGrid1d::RpGrid1d(DataValType* val, int nitems)
{
	int sz = m_data.size();

	if (val == NULL) // invalid data pointer
		return;

	if (sz != nitems)
		m_data.resize(nitems);

	// copy array val into m_data
	sz = nitems;
	void* ptr = &(m_data[0]);
	memcpy(ptr, (void*)val, sizeof(DataValType)*sz);
}

//
// Add all points to grid1d object at once
//
RP_ERROR 
RpGrid1d::addAllPoints(DataValType* val, int nitems)
{
	if (val == NULL)
		return RP_ERR_NULL_PTR;

	if (nitems != numPoints())
		m_data.resize(nitems);

	for (int i=0; i < nitems; i++)
		m_data[i] = val[i];

	return RP_SUCCESS;
}

//
// add one point to grid1d
//
void RpGrid1d::addPoint(DataValType val)
{
	m_data.push_back(val);
}

//
// serialize Grid1d object 
// 16 bytes: 	Header (including RV identifier, version, object type id)
// 				e.g., RV-A-FIELD, RV-A-MESH3D
// 4 bytes:	N: number of bytes in object name (to follow)
// N bytes: 	object name (e.g., "output.grid(g1d))"
// 4 bytes:  	number of points in array
// rest:     	data array (x x x...)
//
char* 
RpGrid1d::serialize(int& nb)
{
	char* buf;
	int nbytes = numBytes();

	if ( (buf = new char[nbytes]) == NULL) {
		RpAppendErr("RpGrid1d::serialize: new char[] failed");
		RpPrintErr();
		return buf;
	}

	doSerialize(buf, nbytes);

	nb = nbytes;

	return buf;
}

// 
// serialize object data in Little Endian byte order
//
RP_ERROR
RpGrid1d::doSerialize(char* buf, int nbytes)
{
	// check buffer 
	if (buf == NULL || nbytes < numBytes()) {
                RpAppendErr("RpGrid1d::serialize: invalid buffer");
		RpPrintErr();
		return RP_ERR_INVALID_ARRAY;
	}

	char * ptr = buf;

	// write object header (version and typename)
	writeRpHeader(ptr, RpGrid1d_current_version, nbytes);
	ptr += HEADER_SIZE + sizeof(int);
	
	// write object name and its length
	writeString(ptr, m_name);
	ptr += m_name.size() + sizeof(int);

	// write number of points and array data
	writeArrayDouble(ptr, m_data, m_data.size());

	return RP_SUCCESS;
}

RP_ERROR 
RpGrid1d::deserialize(const char* buf)
{
	if (buf == NULL) {
		RpAppendErr("RpGrid1d::deserialize: null buf pointer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	char* ptr = (char*)buf;
	std::string header;
	int nbytes;

	readRpHeader(ptr, header, nbytes);
	ptr += HEADER_SIZE + sizeof(int);
	
	if (header == RpGrid1d_current_version)
		return doDeserialize(ptr);

	// deal with older versions
	return RP_FAILURE;
}

// 
// parse out the buffer, 
// stripped off the version and total #bytes already.
//
RP_ERROR
RpGrid1d::doDeserialize(const char* buf)
{
	char* ptr = (char*)buf;

	// parse object name and store name in m_name

	readString(ptr, m_name);
	ptr += sizeof(int) + m_name.size();
	
	int npts;
	readArrayDouble(ptr, m_data, npts);

	return RP_SUCCESS;
}

// 
// return pointer to data points
//
DataValType*
RpGrid1d::getData()
{
	return &m_data[0];
}

//
// return pointer to a copy of data points in consecutive memory locations
//
DataValType*
RpGrid1d::getDataCopy()
{
	int npts = numPoints();

	DataValType* xy;
	if ( (xy = new DataValType[npts]) == NULL) {
                RpAppendErr("RpGrid1d::data: mem alloc failed");
		RpPrintErr();
		return xy;
	}

	for (int i=0; i < npts; i++) 
		xy[i] = m_data[i];

	return xy;
}

//
// mashalling object into xml string
//
void 
RpGrid1d::xmlString(std::string& textString)
{
	int i;
	int npts = m_data.size();
	char cstr[256];

	// clear input string
	textString.erase();

	textString.append("<value>");

	for (i=0; i < npts; i++) {
		sprintf(cstr, "\t%.15f\n", m_data[i]);
		textString.append(cstr);
	}
	textString.append("</value>\n");
}

//
// print the xml string from the object
//
void 
RpGrid1d::print()
{
	string str;

	printf("object name: %s\n", m_name.c_str());

	xmlString(str);

	printf("%s", str.c_str());
}

//
// set object name from a charactor string
//
void 
RpGrid1d::objectName(const char* str)
{
	m_name.assign(str);
}

//
// get object name
//
const char* RpGrid1d::objectName()
{
	return m_name.c_str();
}

//
// get object type
//
const char* RpGrid1d::objectType()
{
	return RpObjectTypes[GRID1D];
}

//
// return total number of bytes when mashalling out as bytes
// 
int RpGrid1d::numBytes()
{
	int nbytes = HEADER_SIZE
		+ sizeof(int) // total #bytes
		+ sizeof(int) // #bytes in name
		+ m_name.size()
		+ sizeof(int) // #points in grid
		+ m_data.size() * sizeof(DataValType);

	return nbytes;
}

