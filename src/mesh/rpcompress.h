#ifndef _RP_COMPRESSION_
#include <iostream>
#include <vector>
#include "zlib.h"
#include "rp_types.h"

using namespace std;

class RpCompressor {
public:
	static int compress(vector<char>& src, vector<char>& dst);
	static int decompress(vector<char>& src, vector<char>& dst);
};

#endif

