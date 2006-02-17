#ifndef __RP_ELEMENT_H__
#define __RP_ELEMENT_H__

#include <iostream>
#include <vector>
#include "node.h"
#include "util.h"


class RpElement {
public:
        // constructors
        RpElement() {  };

        RpElement(int size, int id):
		m_size(size), 
		m_id(id)
	{  
		m_nodes.resize(size);
	};

	// add a node to element
	void addNode(RpNode& node);

	// add node name - may remove this later
	void addNode(int nodeName);

	// add all nodes to element
	void setNodes(RpNode* list, int numNodes, int id);

	// set node names
	RP_ERROR set(int* nodes, int numNodes, int id);
	
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
	int m_size;
	int m_id;
	vector<int> m_nodes;
	vector<RpNode> m_list;
};

#endif
