#ifndef __RP_NODE_H__
#define __RP_NODE_H__

#include <iostream>
#include <vector>
#include "util.h"


class RpNode {
public:
        // constructors (default to 2d node)
        RpNode(int dim=2):
		m_dim(dim)
	{ 
		m_node.resize(dim);
	}

	// assign value to coordinate[index]
	RP_ERROR set(int val, int index);

	char* serialize();
	void serialize(const char* buf, int buflen);
	void deserialize(const char* buf);

        void print();

        virtual ~RpNode() { };

private:
	vector<int> m_node;
	int m_dim;
};

#endif
