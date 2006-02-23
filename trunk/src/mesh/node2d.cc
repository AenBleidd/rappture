#include "node2d.h"

static const int NodeDim = 2;
//
// - serialize node (x y) 
// - store byte stream in buf
//
// Return: 
// 	NULL: fail to allocate memory
// 	valid pointer: point to buffer holding the byte stream
//
char* 
RpNode2d::serialize()
{
	int len = NodeDim * sizeof(int);

	char * buf = new char[len];
	serialize(buf, len);

	return buf;
}

//
// actual function that does the serialization
//
// Input:
// 	buf: pointer to valid memory
// 	len: number of bytes in buf
// Return:
// 	error code for null pointer (defined in "util.h")
// 	success
//
RP_ERROR 
RpNode2d::serialize(char* buf, int buflen)
{
	if (buf == NULL || buflen < NodeDim*((signed)sizeof(int))) {
		RpAppendErr("RpElement::deserialize: null buffer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	char * ptr = buf;
	memcpy((void*)ptr, (void *)&m_x, sizeof(int));
	ptr += sizeof(int);
	memcpy((void*)ptr, (void *)&m_y, sizeof(int));

	return RP_SUCCESS;
}

RP_ERROR
RpNode2d::deserialize(const char* buf)
{
	if (buf == NULL) {
		RpAppendErr("RpElement::deserialize: null buffer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	char * ptr = (char*)buf;
	memcpy((void*)&m_x, (void *)ptr, sizeof(int));
	ptr += sizeof(int);
	memcpy((void*)&m_y, (void *)ptr, sizeof(int));
	
	return RP_SUCCESS;
}

void 
RpNode2d::print()
{
	printf("<node id=\"%d\"> %d %d </node>\n",m_id,m_x,m_y);
}

