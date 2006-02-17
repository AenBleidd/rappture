#ifndef __RP_NODE_H__
#define __RP_NODE_H__

#include <iostream>
#include <vector>
#include "util.h"


class RpNode {
public:
        // constructors (default to 2d node)
        RpNode(int dim=2, const char* id=NULL):
		m_dim(dim),
		m_id(id)
	{ 
		m_node.resize(dim);
	}

	// assign value to coordinate[index]
	RP_ERROR set(int val, int index);
	RP_ERROR get(int& val, int index);

	void setNode2d(int x, int y, const char* id);
	void setNode3d(int x, int y, int z, const char* id);

	const char* id() { return m_id.c_str(); };
	void id(const char* id) { m_id = id; };

	char* serialize();
	void serialize(const char* buf, int buflen);
	void deserialize(const char* buf);

        void print();

        virtual ~RpNode() { };

private:
	vector<int> m_node;
	int m_dim;
	std::string m_id;
};

#endif
