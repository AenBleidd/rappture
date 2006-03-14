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

//
// instantiate grid with a 1d array of doubles and number of items
//
RpGrid1d::RpGrid1d(DataValType* val, int nitems)
{
	if (val == NULL) // invalid data pointer
		return;

	// copy array val into m_data
	for (int i=0; i<nitems; i++)
		m_data.push_back(val[i]);
}

//
// constructor for a regular grid
//
RpGrid1d::RpGrid1d(DataValType startPoint, DataValType delta, int npts)
{
	// expand array
	for (int i=0; i < npts; i++)
		m_data.push_back(startPoint + i*delta);
}


//
// Add all points to grid1d object at once
//
RP_ERROR 
RpGrid1d::addAllPoints(DataValType* val, int nitems)
{
	if (val == NULL)
		return RP_ERR_NULL_PTR;

	m_data.clear();
	m_data.reserve(nitems);

	for (int i=0; i < nitems; i++)
		m_data.push_back(val[i]);

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
	writeRpHeader(ptr, RpCurrentVersion[GRID1D], nbytes);
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
	
	if (header == RpCurrentVersion[GRID1D])
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
	int nitems = m_data.size();

	DataValType* xy = new DataValType[nitems];
	if ( xy == NULL) {
                RpAppendErr("RpGrid1d::data: new failed");
		RpPrintErr();
		return xy;
	}

	for (int i=0; i < nitems; i++) 
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
	textString.clear();

	textString.append("<points>");

	for (i=0; i < npts; i++) {
		sprintf(cstr, "\t%.15f\n", m_data[i]);
		textString.append(cstr);
	}
	textString.append("</points>\n");
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

//
// Remove all points
//
void RpGrid1d::clear()
{
	m_data.clear();
	m_name.clear();
}

//
// override operator = 
//
RpGrid1d RpGrid1d::operator=(const RpGrid1d& g)
{
	m_data = g.m_data;
	m_name = g.m_name;

	return (*this);
}

//
// retrieve x y values at index
//
DataValType RpGrid1d::getData(int index)
{
	return m_data.at(index);
}

