#ifndef __RP_NODE_H__
#define __RP_NODE_H__

#include <iostream>
#include <vector>
#include "util.h"


class RpNode {
public:
        // constructors, default size=3
        RpNode()
	{ 
		m_node.resize(3);
	};

        RpNode(int dim, int id):
		m_dim(dim),
		m_id(id)
	{ 
		m_node.resize(dim);
	}

	// assign value to coordinate[index]
	RP_ERROR set(int val, int index);

	// get node values
	RP_ERROR get(int& val, int index);
	RP_ERROR get(int* val, int len);

	void setNode(int dim, int* val, int id);

	int id() { return m_id; };
	void id(int id) { m_id = id; };

	char* serialize();
	void serialize(const char* buf, int buflen);
	void deserialize(const char* buf);

        void print();

        virtual ~RpNode() { };

private:
	int m_dim;
	int m_id;
	vector<int> m_node;
};

#endif
