#include "rpcompress.h"

int 
RpCompressor::compress(vector<char>& src, vector<char>& dst)
{
	int srcSize = src.size();
	int dstSize = compressBound(srcSize); // zlib computes upper bound

	//cout<< "compress bound: "<<compressBound(srcSize)<<endl;

	// set dst vector size 
	// (4 extra bytes to record number of bytes in original data)
	dst.resize(dstSize + 4);

	// write number of bytes in original data to dst
	writeInt(&dst[0], srcSize);

	unsigned long size = srcSize;
	unsigned long newsize = dstSize;

	// call zlib to compress
	int err = compress2((Bytef*)&dst[4], &newsize, (Bytef*)&src[0], size, 5);
	switch(err) {
		case Z_MEM_ERROR: 
			cout << "zlib compress failed (memory error)" << endl;
			break;
		case Z_BUF_ERROR: 
			cout<<"zlib compress failed (dest buf too small)" <<endl;
			break;
		case Z_OK: 
			cout<<"zlib compress OK ("<<newsize <<" bytes)"<<endl;
			dst.resize(newsize+4);
			break;
		default:
			break;
	}

	return err;
}

// decompress a byte stream in src 
//
// expanded data stored in dst
//
// first 4 bytes in src is the number of bytes in orginal, 
// (uncompressed) byte stream
//
int 
RpCompressor::decompress(vector<char>& src, vector<char>& dst)
{
	unsigned long size = src.size();

	// read orginal size from src - zlib uncompress requires it
	int n;
	readInt(&src[0], n);
	dst.resize(n); // set dst vector size

	unsigned long newsize = n;

	// call zlib to decompress
	int err = uncompress((Bytef*)&dst[0], &newsize, (Bytef*)&src[4], size);
	switch(err) { 
		case Z_MEM_ERROR: 
			cout << "zlib uncompress memory error" << endl;
			break;
		case Z_BUF_ERROR: 
			cout<<"zlib uncompress dest buf too small" <<endl;
			break;
		case Z_OK: 
			cout<<"zlib uncompress OK ("<<newsize <<" bytes)"<<endl;
			dst.resize(newsize);
			break;
		default:
			break;
	}

	return err;
}
