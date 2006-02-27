#ifndef __RP_MESH3D_H__
#define __RP_MESH3D_H__

#include <string>
#include "element.h"
#include "util.h"


class RpMesh3d {
public:
        // constructor
        RpMesh3d(const char* id, int numNodes, int numElements, 
			int nodesInElement);

	int numNodes() { return m_numNodes; };
	int numElements() { return m_numElements; };
	const char* id() { return m_id.c_str(); };

	// want user to set these numbers in constructor
	//void numNodes(int n) { m_numNodes = n; };
	//void numElements(int n) { m_numElements = n; };
	//void id(const char* id) { m_id = id; };

	// add all nodes to mesh
	RP_ERROR addAllNodes(int numNodes, int* nodesList[]);

	// add one node to mesh
	RP_ERROR addNode(int* nodesList);

	// add an element to mesh
	RP_ERROR addElement(int numNodesInElem, int* nodes);

	// retrieve nodes 
	void getNode(int nodeSeqNum, int& x, int& y, int& z);
	RpNode3d getNode(int nodeSeqNum);
	RP_ERROR getNodesList(int* nodesList, int& num);

	RpElement getElement(int elemSeqNum);
	RP_ERROR getElement(int elemSeqNum, int* nodesBuf);

	// serialization
	char* serialize();
	RP_ERROR serialize(char* buf, int buflen);
	RP_ERROR deserialize(const char* buf);

        void print();

        virtual ~RpMesh3d();

protected:
	// default constructor
        RpMesh3d() { };

private:
	std::string m_id;
	int m_numNodes;
	int m_numElements;
	int m_nodeIndex;
	int m_elemIndex;
	vector<RpNode3d> m_nodeList; // list of node objects
	vector<RpElement> * m_elemList; // ptr to a list of RpElement objects
};

#endif
