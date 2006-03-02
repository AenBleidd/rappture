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
	m_data.reserve(size);
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

//
// Add all points to grid1d object at once
//
RP_ERROR 
RpGrid1d::addAllPoints(double* val, int nitems)
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
RpGrid1d::serialize(int& nb, RP_ENCODE_ALG encodeFlag, 
				RP_COMPRESSION compressFlag)
{
	int npts = m_data.size(); 
	int nbytes = npts*sizeof(double) + sizeof(int) + 2;
	char* buf;

	// total length = tagEncode + tagCompress + num + array data 
	if ( (buf = (char*) malloc(nbytes)) == NULL) {
		RpAppendErr("RpGrid1d::serialize: malloc failed");
		RpPrintErr();
		return buf;
	}

	serialize(buf, nbytes, encodeFlag, compressFlag);
	nb = nbytes;

	return buf;
}

RP_ERROR
RpGrid1d::serialize(char* buf, int nbytes, RP_ENCODE_ALG encodeFlag, 
		RP_COMPRESSION compressFlag)
{
	int npts = m_data.size();

	if (buf == NULL || (unsigned)nbytes < (npts*sizeof(double)+sizeof(int)+2)) {
                RpAppendErr("RpGrid1d::serialize: invalid buffer");
		RpPrintErr();
		return RP_ERR_INVALID_ARRAY;
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
	ptr += 2; // advance pointer

	// TODO encode, compression
	// compress()

	// write to stream buffer
	memcpy((void*)ptr, (void*)&npts, sizeof(int));
	ptr += sizeof(int);
	memcpy((void*)ptr, (void*)&(m_data[0]), npts*sizeof(double));

	return RP_SUCCESS;
}

RP_ERROR 
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

void 
RpGrid1d::xmlString(std::string& textString)
{
	int i;
	int npts = m_data.size();
	char cstr[256];
	std::string str;

	// clear input string
	textString.erase();

	textString.append("<value>");

	for (i=0; i < npts; i++) {
		sprintf(cstr, "\t%.15f\n", m_data[i]);
		textString.append(cstr);
	}
	textString.append("</value>\n");
}

void 
RpGrid1d::print()
{
	string str;
	xmlString(str);
	printf("%s", str.c_str());
}


// TODO
//int RpGrid1d::xmlPut() { };
//int RpGrid1d::xmlGet() { };

