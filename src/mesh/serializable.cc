#include "serializable.h"

//
// parse the header from byte stream
// pass version/type string and number of bytes back to caller
//
void RpSerializable::readHeader(const char* buf, string& ver, int& nbytes)
{
	char* ptr = (char*)buf;

	// read header
	char* header = new char[HEADER_SIZE];
	ByteOrder<char>::OrderCopyArray(ptr, header, HEADER_SIZE);
	filterTrailingBlanks(header, HEADER_SIZE);
	ver = header;

	ptr += HEADER_SIZE;

	// read total number of bytes
	int* iptr = (int*)ptr;
	ByteOrder<int>::OrderCopy(iptr, &nbytes);
#ifdef DBEUG
	printf("readHeader: header=%s=, nbytes=%d\n, header, nbytes);
#endif

	delete header;
}

// 
// write out the version/type string and number of bytes
//
void RpSerializable::writeHeader(char* buf, const char* ver, int nbytes)
{
	char* ptr = buf;

	// prepare header
	// fill in blanks if version string is shorter than HEADER_SIZE

	int len = strlen(ver);
	std::string header(' ', HEADER_SIZE);
	header = ver;
	header.append(HEADER_SIZE-len, ' ');

	// copy header 
	const char* cptr = header.c_str();
	ByteOrder<char>::OrderCopyArray(cptr, (char*)ptr, HEADER_SIZE);
	ptr += HEADER_SIZE;

	// copy total number of bytes: nbytes
	int* iptr = (int*)ptr;
	ByteOrder<int>::OrderCopy(&nbytes, iptr);
}

// 
// parse object name
//
void RpSerializable::readObjectName(const char* buf, string& name)
{
	char* ptr = (char*)buf;
	int len;

	// copy length of name

	ByteOrder<int>::OrderCopy((int*)ptr, &len);

	ptr += sizeof(int);
	
	// copy name as chars

	char* cstr = new char[len]; ;
	ByteOrder<char>::OrderCopyArray(ptr, cstr, len);
	filterTrailingBlanks(cstr, len);
	name = cstr;

	delete cstr;
}

void 
RpSerializable::writeObjectName(char* buf, string& name)
{
	char *ptr = buf;

	// copy length of name

	int len = name.size();
	printf("writeObjectName: len=%d name=%s=\n", len, name.c_str());

	ByteOrder<int>::OrderCopy(&len, (int*)ptr);
	
	ptr += sizeof(int);
	
	// copy name as chars
	const char* cptr = name.c_str();
	ByteOrder<char>::OrderCopyArray(cptr, ptr, len);
}

void 
RpSerializable::readArrayDouble(const char* buf, vector<double>* data, int& npts)
{
	int* iptr = (int*)buf;
	char* ptr = (char*)buf;

	// read number of points

	ByteOrder<int>::OrderCopy(iptr, &npts);
	ptr += sizeof(int);

	// set the array to be the right size
	if (data->size() < (unsigned)npts)
		data->resize(npts);

#ifdef DEBUG
	printf("readArrayDouble: data size %d\n", data->size());
#endif

	// copy points array - use ByteOrder copy
	double* dptr = (double*)ptr;
	ByteOrder<double>::OrderCopyArray(dptr, &((*data)[0]), npts);

}

void 
RpSerializable::writeArrayDouble(char* buf, vector<double>& data, int npts)
{
	char * ptr = buf;

	// write int (number of points)
	int* iptr = (int*)ptr;
	ByteOrder<int>::OrderCopy(&npts, iptr);
	ptr += sizeof(int);
#ifdef DEBUG
	printf("writeArrayDouble: 1st data=%.12f addr=%x\n", data[0], 
			(unsigned int)&(data[0]));
#endif

	// write array
	double * dptr = (double*)ptr;	
	ByteOrder<double>::OrderCopyArray(&(data[0]), dptr, npts);
}
