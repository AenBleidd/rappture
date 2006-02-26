#ifndef __RP_ELEMENT_H__
#define __RP_ELEMENT_H__

#include <iostream>
#include <vector>
#include "util.h"

//
// RpElement is defined as a list of nodes (by id) connected with each other
// in a mesh.
//
class RpElement {
public:

	// constructor, set number of nodes and/or element id
        RpElement(int numNodes, int id=0);

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

	int numNodes() { return m_nodes.size(); }; 

	// serialize
	// id(int), numNodes(int), list of ids(int *)
	char* serialize();
	RP_ERROR serialize(char* buf, int buflen);
	RP_ERROR deserialize(const char* buf);

	// serialize RpElement object into xml text
        void xmlString(std::string& xmlText);
        void print();

        virtual ~RpElement() { };

	// TODO
	// void xmlPut();
	// void xmlGet();

protected:
        // constructors, default to 8 nodes
        RpElement();

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
