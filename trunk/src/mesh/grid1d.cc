//
// class for 1D grid
//

#include "grid1d.h"


// constructor
RpGrid1d::RpGrid1d()
{
}

RpGrid1d::RpGrid1d(int size)
{
	m_data.resize(size);
}

// construct a grid object from a byte stream
RpGrid1d::RpGrid1d(const char * buf)
{
	deserialize(buf);
}

//
// instantiate grid with a 1d array of doubles and number of items
//
RpGrid1d::RpGrid1d(double* val, int nitems)
{
	int sz = m_data.size();

	if (val == NULL) // invalid data pointer
		return;

	if (sz != nitems)
		m_data.resize(nitems);

	// copy array val into m_data
	sz = nitems;
	void* ptr = &(m_data[0]);
	memcpy(ptr, (void*)val, sizeof(double)*sz);
}

void RpGrid1d::addPoint(double val)
{
	m_data.push_back(val);
}

// serialize object 
// 1 byte:   tag indicating if data is uuencoded
// 1 byte:   tag indicating compression algorithm (N: NONE, Z: zlib)
// 4 bytes:  number of points in array
// rest:     data array (x x x...)
//
// TODO - handling Endianess 
//
char* 
RpGrid1d::serialize(RP_ENCODE_ALG encodeFlag, RP_COMPRESSION compressFlag)
{
	int npts = m_data.size(); 

	// total length = tagEncode + tagCompress + num + array data 
	char * buf = (char*) malloc(npts*sizeof(double) + sizeof(int) + 2);
	if (buf == NULL) {
		RpAppendErr("RpGrid1d::serialize: malloc failed");
		RpPrintErr();
		return buf;
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
	ptr += 2;
	memcpy((void*)ptr, (void*)&npts, sizeof(int));
	ptr += sizeof(int);
	memcpy((void*)ptr, (void*)&(m_data[0]), npts*sizeof(double));

	return buf;
}

int 
RpGrid1d::deserialize(const char* buf)
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

	m_data.resize(npts); // set the array to be the right size
	memcpy((void*)&(m_data[0]), (void*)buf, npts*sizeof(double));

	/* TODO
	if (ByteOrder::IsBigEndian()) {
		for (int i=0; i<npts; i++)
		{
			// flip each val
		}
	} */

	return RP_SUCCESS;
}


//TODO: overload operators [], =

// TODO
//int RpGrid1d::xmlPut() { };
//int RpGrid1d::xmlGet() { };

