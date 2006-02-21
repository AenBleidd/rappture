#ifndef __RP_ELEMENT_H__
#define __RP_ELEMENT_H__

#include <iostream>
#include <vector>
#include "node3d.h"
#include "util.h"

//
// RpElement is defined as a list of nodes (by id) connected with each other
// in a mesh.
//
class RpElement {
public:
        // constructors
        RpElement()
	{
		// default
		m_nodes.resize(10);
	};

        RpElement(int numNodes, int id)
	{
		m_id = id;
		if (m_nodes.capacity() < (unsigned)numNodes)
			m_nodes.resize(numNodes);
	};

	// instantiate from byte stream:
	// id(int), numNodes(int), list of node ids(int *)
        RpElement(const char* buf);

	// add a node to element
	void addNode(int nodeName);

	// add all the nodes to element
	RP_ERROR addNodes(const int* nodes, int numNodes);
	
	// get node id's
	RP_ERROR getNodeIdList(int* list, int& len);

	int id() { return m_id; };
	void id(int id) { m_id = id; };


	// serialize
	// id(int), numNodes(int), list of ids(int *)
	char* serialize();
	RP_ERROR serialize(char* buf, int buflen);
	RP_ERROR deserialize(const char* buf);

        void print();

        virtual ~RpElement() { };

	// TODO
	// void xmlPut();
	// void xmlGet();

private:
	int m_id;
	vector<int> m_nodes; // list of node ids

	// total number of bytes in an element:
	//     bytes for all node ids + element id + numberOfNodes
	int numBytes() {
		return ((m_nodes.size()) * sizeof(int) + 2*sizeof(int));
	};
};

#endif
