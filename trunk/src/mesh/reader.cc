#include "byte_order.h"
#include "util.h"
#include "rp_types.h"

//
// parse the header from byte stream
// pass version/type string and number of bytes back to caller
//
void readRpHeader(const char* buf, string& ver, int& nbytes)
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
	printf("readRpHeader: header=%s=, nbytes=%d\n, header, nbytes);
#endif

	delete [] header;
}

// 
// write out the version/type string and number of bytes
//
void writeRpHeader(char* buf, const char* ver, int nbytes)
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
// parse dynamic length string
// first 4 bytes: length of string = len
// character array of length = len
//
void readString(const char* buf, string& str)
{
	char* ptr = (char*)buf;
	int len;

	// read length of string

	ByteOrder<int>::OrderCopy((int*)ptr, &len);

	ptr += sizeof(int);
#ifdef DEBUG
	printf("readString(const char*,string&): len=%d, ptr=%x\n",
			len, (unsigned)ptr);
#endif
	
	// read chars

	char* cstr = new char[len+1]; ;
	cstr[len] = '\0';

	ByteOrder<char>::OrderCopyArray(ptr, cstr, len);

#ifdef DEBUG
	printf("readString: str=%s=, %d bytes\n", cstr, len);
#endif

	str.assign((const char*)cstr);

	delete [] cstr;
}

//
// read chars of length(len) from buffer (buf)
// return chars in string (null terminated)
//
void readString(const char* buf, string& str, int len)
{
	char* ch = new char[len];

	ByteOrder<char>::OrderCopyArray(buf, ch, len);
	filterTrailingBlanks(ch, len);

	str = ch;

	delete [] ch;
}

void writeString(char* buf, const char* str, int len)
{
	char *ptr = buf;

	// write length of string

	ByteOrder<int>::OrderCopy(&len, (int*)ptr);
	ptr += sizeof(int);
	
	// write chars
	ByteOrder<char>::OrderCopyArray(str, ptr, len);
}

void writeString(char* buf, string& str)
{
	writeString(buf, str.c_str(), str.size());
}

void readArrayDouble(const char* buf, vector<double>& data, int& npts)
{
	int* iptr = (int*)buf;
	char* ptr = (char*)buf;

	// read number of points

	ByteOrder<int>::OrderCopy(iptr, &npts);
	ptr += sizeof(int);

	// set the array to be the right size
	if (data.size() < (unsigned)npts)
		data.resize(npts);

#ifdef DEBUG
	printf("readArrayDouble: data size %d\n", data.size());
#endif

	// copy points array - use ByteOrder copy
	double* dptr = (double*)ptr;
	ByteOrder<double>::OrderCopyArray(dptr, &(data[0]), npts);
}

void writeArrayDouble(char* buf, vector<double>& data, int npts)
{
#ifdef DEBUG
	printf("writeArrayDouble: buf=%x\n", (unsigned)(buf));
#endif
	char * ptr = buf;

	// write int (number of points)
	int* iptr = (int*)ptr;
	ByteOrder<int>::OrderCopy(&npts, iptr);
	ptr += sizeof(int);
#ifdef DEBUG
	printf("writeArrayDouble: 1st data=%.12f npts=%d addr=%x\n", 
			data[0], npts, (unsigned int)&(data[0]));
#endif

	// write array
	double * dptr = (double*)ptr;	
	ByteOrder<double>::OrderCopyArray(&(data[0]), dptr, npts);
}

void readInt(const char* buf, int& val)
{
	ByteOrder<int>::OrderCopy((int*)buf, &val);
}

void writeInt(char* buf, int val)
{
	ByteOrder<int>::OrderCopy(&val, (int*)buf);
}

void readArrayInt(const char* buf, vector<int>& data, int& npts)
{
	int* iptr = (int*)buf;
	char* ptr = (char*)buf;

	// read number of points

	ByteOrder<int>::OrderCopy(iptr, &npts);
	ptr += sizeof(int);

	// set the array to be the right size
	if (data.size() < (unsigned)npts)
		data.resize(npts);

#ifdef DEBUG
	printf("readArrayInt: data size %d\n", data.size());
#endif

	// copy points array - use ByteOrder copy
	iptr = (int*)ptr;
	ByteOrder<int>::OrderCopyArray(iptr, &(data[0]), npts);
}

void writeArrayInt(char* buf, vector<int>& data, int npts)
{
	char * ptr = buf;

	// write int (number of points)

	int* iptr = (int*)ptr;
	ByteOrder<int>::OrderCopy(&npts, iptr);
	ptr += sizeof(int);

#ifdef DEBUG
	printf("writeArrayInt: 1st data=%d addr=%x\n", data[0], 
			(unsigned int)&(data[0]));
#endif

	// write array
	iptr = (int *)ptr;	
	ByteOrder<int>::OrderCopyArray(&(data[0]), iptr, npts);
}

