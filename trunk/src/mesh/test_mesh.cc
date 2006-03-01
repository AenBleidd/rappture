#include <string>
#include "mesh.h"

#define Num_nodes 8
#define Num_elements 6

static int nodes[] = {
	0,0,0,
	1,0,0,
	1,1,0,
	0,1,0,
	0,1,1,
	1,1,1,
	0,1,1,
	0,0,1
};

static int elem[] = {
	0,1,5,6,
	1,2,6,7,
	2,3,7,1,
	3,4,1,2,
	5,6,2,3,
	6,7,3,4
};

static int nodes2[8][3] = {
	0,0,0,
	1,0,0,
	1,1,0,
	0,1,0,
	0,1,1,
	1,1,1,
	0,1,1,
	0,0,1
};

static int elem2[6][4] = {
	0,1,5,6,
	1,2,6,7,
	2,3,7,1,
	3,4,1,2,
	5,6,2,3,
	6,7,3,4
};


int main()
{
	int i, j, err;
	RpMesh3d * mptr, *meshptr;

	printf("Testing RpMesh3d\n");

	printf("Testing constructor\n");

	mptr = new RpMesh3d("m3d", Num_nodes, Num_elements, 4);
	if (mptr == NULL)
		return 1;

	printf("Testing addAllNodes\n");

	// add nodes to mesh
	err = mptr->addAllNodes(Num_nodes, nodes);
	if (err != RP_SUCCESS) {
		printf("addAllNodes: err code = %d\n", err);
		return 1;
	}

	printf("Testing addAllElements\n");

	err = mptr->addAllElements(Num_elements, elem);
	if (err != RP_SUCCESS) {
		printf("addAllElements: err code = %d\n", err);
		return 1;
	}

	printf("Testing xmlString\n");

	string str;
	mptr->xmlString(str);
	printf("%s", str.c_str());

	printf("Testing getNodesList\n");
	int retNodes[8][3];

	int n = Num_nodes;
	mptr->getNodesList(n, &(retNodes[0][0]));
	for (i=0; i<Num_nodes; i++) {
		printf("%d %d %d\n", 
				retNodes[i][0],
				retNodes[i][1],
				retNodes[i][2]);
	}

	printf("Testing getAllElements\n");
	int retElem[6][4];

	mptr->getAllElements(Num_elements, &(retElem[0][0]));

	n = mptr->elementSize();
	for (i=0; i<Num_elements; i++) {
		for (j=0; j<n; j++)
			printf("%d ", retElem[i][j]);
		printf("\n");
	}

	printf("Testing getElement\n");
	RpElement elem(4);
	elem = mptr->getElement(3);
	elem.print();

	printf("Testing erase\n");
	mptr->erase();
	mptr->print();

	delete mptr;

	printf("Testing addNode\n");

	meshptr = new RpMesh3d("newMesh3d", Num_nodes, Num_elements, 4);
	for (i=0; i<Num_nodes; i++)
		meshptr->addNode(nodes2[i]);
	for (i=0; i<Num_elements; i++)
		meshptr->addElement(4, elem2[i]);

	meshptr->print();

	return 0;
}
