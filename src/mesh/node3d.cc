#include "node3d.h"

//
// - serialize node (x y z) 
// - store byte stream in buf
//
// Return: 
// 	NULL: fail to allocate memory
// 	valid pointer: point to buffer holding the byte stream
//
char* 
RpNode3d::serialize()
{
	int len = 3 * sizeof(int);

	char * buf = new char[len];
	serialize(buf);

	return buf;
}

//
// actual function that does the serialization
//     serialization order:
//     	x y z x y z ...
//
// Input:
// 	buf: pointer to valid memory
// 	len: number of bytes in buf
// Return:
// 	error code for null pointer (defined in "util.h")
// 	success
//
RP_ERROR 
RpNode3d::serialize(char* buf)
{
	if (buf == NULL) {
		RpAppendErr("RpElement::deserialize: null buffer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	char * ptr = buf;
	memcpy((void*)ptr, (void *)&m_x, sizeof(int));
	ptr += sizeof(int);
	memcpy((void*)ptr, (void *)&m_y, sizeof(int));
	ptr += sizeof(int);
	memcpy((void*)ptr, (void *)&m_z, sizeof(int));

	return RP_SUCCESS;
}

RP_ERROR
RpNode3d::deserialize(const char* buf)
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
	ptr += sizeof(int);
	memcpy((void*)&m_z, (void *)ptr, sizeof(int));
	
	return RP_SUCCESS;
}

void 
RpNode3d::xmlString(std::string& str)
{
	char cstr[512];
	sprintf(cstr, "<node id=\"%d\"> %d %d %d</node>\n",m_id,m_x,m_y,m_z);
	str.append(cstr);
}

void 
RpNode3d::print()
{
	printf("<node id=\"%d\"> %d %d %d</node>\n",m_id,m_x,m_y,m_z);
}

