#ifndef __RP_ELEMENT_H__
#define __RP_ELEMENT_H__

#include <iostream>
#include <vector>
#include "node.h"
#include "util.h"

//
// RpElement is defined as a list of nodes (by id) connected with each other
// in a mesh.
//
// Example:
// 	int nodeIDs[8];
// 	RpElement elements[10];
//
//      // set ids for all elements
// 	for (int i=0; i<10; i++)
// 		elements[i].id(i);
//
//	// create node list for element 0
// 	nodeIDs[0] = 0;
// 	nodeIDs[1] = 1;
// 	nodeIDs[2] = 5;
// 	nodeIDs[3] = 6;
// 	nodeIDs[4] = 25;
// 	nodeIDs[5] = 26;
// 	nodeIDs[6] = 30;
// 	nodeIDs[7] = 31;
// 	element[0].addNodes(nodeIDs, 8);
//
// 	nodeIDs[0] = 1;
// 	nodeIDs[1] = 2;
// 	nodeIDs[2] = 6;
// 	nodeIDs[3] = 7;
// 	nodeIDs[4] = 26;
// 	nodeIDs[5] = 27;
// 	nodeIDs[6] = 31;
// 	nodeIDs[7] = 32;
// 	element[1].addNodes(nodeIDs, 8);
//
//      ... 
//
//

class RpElement {
public:
        // constructors
        RpElement()
	{
		// default
		m_nodes(8);
	};

        RpElement(int numNodes, int id)
	{
		m_id = id;
		if (m_nodes.capacity() < numNodes)
			m_nodes.resize(numNodes);
	};

	// instantiate from byte stream:
	// id(int), numNodes(int), list of ids(int *)
        RpElement(const char* buf);

	// add a node to element
	void addNode(int nodeName);

	// add all the nodes to element
	RP_ERROR addNodes(const int* nodes, int numNodes);
	
	// get node id's
	RP_ERROR getNodeIdList(int* list, int len);

	int id() { return m_id; };
	void id(int id) { m_id = id; };

	char* serialize();
	void serialize(const char* buf, int buflen);
	void deserialize(const char* buf);

        void print();

        virtual ~RpElement() { };

	// TODO
	// void xmlPut();
	// void xmlGet();

private:
	int m_id;
	vector<int> m_nodes; // list of node ids
};

#endif
