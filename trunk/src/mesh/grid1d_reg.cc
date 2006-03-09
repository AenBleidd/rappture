//
// class for a regular 1D grid, 
// defined by  a starting point and delta increment.
// 
//

#include "grid1d_reg.h"

RpGrid1d_reg::RpGrid1d_reg(const char* buf)
{
	deserialize(buf);
}


//
// serialize object
// 4 Bytes:  number of points in grid
// 8 Bytes:  start value
// 8 Bytes:  delta 
//
// Returns: pointer to byte stream
// Side effect: caller must release memory 
//
// TODO: handling 64-bit systems
//
char * 
RpGrid1d_reg::serialize()
{
	// buffer size
	char * buf = (char*) new char[sizeof(int) + 2*sizeof(double) + 2];

	if (buf == NULL) {
		RpAppendErr("RpGrid1d::serialize: new failed");
		RpPrintErr();
		return buf;
	}       

	char* ptr = &(buf[2]); // skip 2 bytes (encode/compression flag)

	memcpy((void*)ptr, (void*)&m_npoints, sizeof(int));
	ptr += sizeof(int);
	memcpy((void*)ptr, (void*)&m_start, sizeof(double));
	ptr += sizeof(double);
	memcpy((void*)ptr, (void*)&m_delta, sizeof(double));

	return buf;
}

//
// deserialize byte stream 
//
// Returns: error code (RP_SUCCESS or )
// Side effect: buf is NOT deleted. Caller must release buf.
//
int 
RpGrid1d_reg::deserialize(const char* buf)
{
	int num;
	double vstart, vdelta;

	// check for null pointer
	if (buf == NULL) {
		//RpAppendErr("RpGrid1d::deserialize: null buf pointer");
		//RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	buf += 2; // skip 2 bytes (encode/compression) TODO

	memcpy((void*)&num, (void*)&buf, sizeof(int));
	buf += sizeof(int);
	memcpy((void*)&vstart, (void*)buf, sizeof(double));
	buf += sizeof(double);
	memcpy((void*)&vdelta, (void*)buf, sizeof(double));

	// TODO
	// if (ByteOrder::IsBigEndian()) {
	//      flip
	// }
	
	// little endian
	m_npoints = num;
	m_start = vstart;
	m_delta = vdelta;

	return RP_SUCCESS;
}

// 
// access data elements
//
// Returns: pointer to an array of doubles
//
double *
RpGrid1d_reg::data()
{
	m_data.resize(m_npoints);

	// expand array
	for (int i=0; i<m_npoints; i++)
		m_data[i] = m_start + i * m_delta;

	return &m_data[0];
}


// TODO
//virtual int xmlPut() { };
//virtual int xmlGet() { };

