#include "node3d.h"
#include "element.h"

#define Num_nodes 8
#define Num_elements 6

static int nodes[8][3] = {
	0,0,0,
	1,0,0,
	1,1,0,
	0,1,0,
	0,1,1,
	1,1,1,
	0,1,1,
	0,0,1
};

static int elem[6][4] = {
	0,1,5,6,
	1,2,6,7,
	2,3,7,1,
	3,4,1,2,
	5,6,2,3,
	6,7,3,4
};

int main()
{
	// create a list of elements
	// each element represents connectivity of 4 nodes
	//
	vector<RpElement> elemList(Num_elements, RpElement(4));

	// an array of nodes
	RpNode3d nodeList[Num_nodes];

	int i;

	// add nodes to nodeList
	for (i=0; i<Num_nodes; i++) {
		nodeList[i].set(nodes[i]);
		nodeList[i].id(i);
		nodeList[i].print();
	}

	string str;
	//elemList = new RpElement[Num_elements];

	for (i=0; i<Num_elements; i++) {
		// each element has 4 nodes
		(elemList[i]).id(i);
		elemList[i].addNodes(elem[i], 4);
		elemList[i].xmlString(str);
		printf("%s\n", str.c_str());
		printf("element size: %d\n", elemList[i].numNodes());
	}
	
	return 0;
}
