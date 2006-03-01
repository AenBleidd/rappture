#include <string>
#include "mesh.h"

#define Num_nodes 8
#define Num_elements 6

static int nodes[8][3] = {
	{0,0,0},
	{1,0,0},
	{1,1,0},
	{0,1,0},
	{0,1,1},
	{1,1,1},
	{0,1,1},
	{0,0,1}
};

static int elem[6][4] = {
	{0,1,5,6},
	{1,2,6,7},
	{2,3,7,1},
	{3,4,1,2},
	{5,6,2,3},
	{6,7,3,4}
};

int main()
{
	int i, err;
	RpMesh3d * mptr;

	mptr = new RpMesh3d("m3d", Num_nodes, Num_elements, 4);
	if (mptr == NULL)
		return 1;

	// add nodes to mesh
	err = mptr->addAllNodes(Num_nodes, &(nodes[0][0]));
	if (err != RP_SUCCESS) {
		printf("addAllNodes: err code = %d\n", err);
		return 1;
	}

	err = mptr->addAllElements(Num_elements, &(elem[0][0]));
	if (err != RP_SUCCESS) {
		printf("addAllElements: err code = %d\n", err);
		return 1;
	}

	string str;
	mptr->xmlString(str);
	printf("%s", str.c_str());

	return 0;
}
