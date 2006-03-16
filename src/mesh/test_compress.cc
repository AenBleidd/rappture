#include <iostream>
#include <fstream>
#include "rpcompress.h"

static double points[20] = {
	0.261243291268,
	0.159055762008,
	0.214550893827,
	0.11741510008,
	0.119735699467,
	0.15196145742,
	0.0245663797288,
	0.128903081711,
	0.0927746958394,
	0.0465364541799,
	0.531606236106,
	0.252448742721,
	0.348575614391,
	0.180939456908,
	0.0251118046222,
	0.810354715199,
	0.0980414196039,
	0.392354903151,
	0.151346474849,
	0.368703495654
};

/*
 * This test program reads a file into memory, compress and writes
 * the deflated data into a new file.
 */
int main(int argc, char* argv[])
{
	vector<char> src, dst;

	// create data
	vector<double> data;
	for (int i=0; i<20; i++)
		//data.push_back(points[i]);
		data.push_back(i);

	src.resize(20*sizeof(double));
	writeArrayDouble(&src[0], data, 20);
	cout<<"src size: " << src.size() <<endl;

	RpCompressor::compress(src, dst);

	cout<<"dst size: " << dst.size() << endl;

	// restore data to newbuf
	vector<char> newbuf;
	if (RpCompressor::decompress(dst, newbuf)) {
		cout<< "error decompress\n";
		return  1;
	}

	if (src == newbuf)
		cout<< "src equals newbuf\n";
}
