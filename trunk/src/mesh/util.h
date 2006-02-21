#ifndef __RP_DEFS_H__
#define __RP_DEFS_H__

#include <iostream>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE	1
#endif

enum DATA_VALUE_TYPE { 
	TYPE_UNKNOWN,
	TYPE_INT,
	TYPE_UINT,
        TYPE_FLOAT,
	TYPE_DOUBLE
};

enum RP_ERROR { 
	RP_SUCCESS,
	RP_FAILURE,
	RP_ERR_MEM_ALLOC,
	RP_ERR_NULL_PTR,
	RP_ERR_TIMEOUT,
	RP_ERR_OUTOFBOUND_INDEX,
	RP_ERR_INVALID_ARRAY
};

enum RP_ENCODE_ALG { 
	RP_NO_ENCODING,
	RP_UUENCODE
};

enum RP_COMPRESSION { 
	RP_NO_COMPRESSION,
	RP_ZLIB
};

using namespace std;

extern string RpErrorStr;

extern void RpPrintErr();
extern void RpAppendErr(const char* msg);

#endif
