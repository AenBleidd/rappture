//
// class for 2D grid
//

#include "grid2d.h"

// Constructors
RpGrid2d::RpGrid2d()
{
}

RpGrid2d::RpGrid2d(int npoints)
{
	m_data.reserve(npoints*2);
}

// constructor
// Input: 
// 	val: array of x y values (x1 y1 x2 y2 ...)
// 	number of points
//
RpGrid2d::RpGrid2d(DataValType* val, int npoints)
{
	m_data.reserve(npoints*2);

	for (int i=0; i<2*npoints; i++)
		m_data[i] = val[i];
}

//
// constructor
// Input:
// 	xval: array of x values
// 	yval: array of y values
// 	npoints: number of points
//
RpGrid2d::RpGrid2d(DataValType* xval, DataValType* yval, int npoints)
{
	m_data.resize(npoints*2);

	for (int i=0; i<npoints; i++)
		addPoint(xval[i], yval[i]);
}

// instantiate with byte stream
RpGrid2d::RpGrid2d(const char* buf)
{
	deserialize(buf);
}


//
// add a point (x, y) pair to grid
//
void RpGrid2d::addPoint(DataValType x, DataValType y)
{
	m_data.push_back(x);
	m_data.push_back(y);
}

//
// access data as a 1d array: x y x y x y...
//
DataValType* RpGrid2d::data()
{
	return (DataValType*) &(m_data[0]);
}

//
// serialize data 
// 1 byte: encoding 
// 1 byte: compression
// 4 bytes: number of points
// rest:   data points, x y x y x y
//
char * 
RpGrid2d::serialize(RP_ENCODE_ALG encodeFlag, RP_COMPRESSION compressFlag)
{
	int numVals = m_data.size(); 
	int npts = numVals/2;

	// total length = tagEncode + tagCompress + num + data
	char * buf = (char*) new char[numVals*sizeof(DataValType) + sizeof(int) + 2];
	if (buf == NULL) {
		RpAppendErr("RpGrid2d::serialize: malloc failed");
		RpPrintErr();
		return NULL;
	}

	char* ptr = buf;
	
	ptr[0] = 'N'; // init to no-encoding
	switch(encodeFlag) {
		case RP_UUENCODE:
			ptr[0] = 'U';
			break;
		case RP_NO_ENCODING:
		default:
			break;
	}

	ptr[1] = 'N'; // init to no compression
	switch(compressFlag) {
		case RP_ZLIB:
			ptr[1] = 'Z';
			break;
		case RP_NO_COMPRESSION:
		default:
			break;
	}

	// TODO encode, compression
	//

	// write to stream buffer
	ptr += 2; // skip first two bytes

	memcpy((void*)ptr, (void*)&npts, sizeof(int));
	ptr += sizeof(int);

	memcpy((void*)ptr, (void*)&(m_data[0]), numVals*sizeof(DataValType));

	return buf;
}

int RpGrid2d::deserialize(const char* buf)
{
	int npts;

	if (buf == NULL) {
		RpAppendErr("RpGrid1d::deserialize: null buf pointer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	// TODO: handle encoding, decompression
	
	buf += 2; // skip 1st 2 bytes

	// read number of points
	memcpy((void*)&npts, (void*)buf, sizeof(int));
	buf += sizeof(int);

	m_data.resize(npts*2); // set the array to be the right size
	memcpy((void*)&(m_data[0]), (void*)buf, npts*2*sizeof(DataValType));

	/* TODO
	if (ByteOrder::IsBigEndian()) {
		for (int i=0; i<npts; i++)
		{
			// flip each x
			// flip each y
		}
	} */

	return RP_SUCCESS;
}


// TODO
//int RpGrid2d::xmlPut() { };
//int RpGrid2d::xmlGet() { };



