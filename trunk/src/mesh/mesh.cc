#include "mesh.h"

/*
   private:
	std:string m_id;
	int m_numNodes;
	int m_numElements;
	int m_nodeIndex;
	int m_elemIndex;
	vector<RpNode3d> m_nodeList;
	RpElement **m_elemList; // list of pointers to RpElement objects
*/

// constructor
// Input:
// 	number of nodes in mesh
// 	number of elements in mesh
//
// Side effect:
// 	memory for nodes and elements is allocated for mesh
//
RpMesh3d::RpMesh3d(const char* id, int numNodes, int numElements, int numNodesInElement)
{
	m_id = id;
	m_numNodes = numNodes;
	m_numElements = numElements;

	// allocate memory for list of nodes
	m_nodeList.resize(numNodes);
	m_nodeIndex = 0;

	// allocate memory for element list
	m_elemList = new vector<RpElement> (numElements,RpElement(numNodesInElement));
	m_elemIndex = 0;
}

//
// Add all the nodes to mesh
// Input:
//    1. nodes: 2-d array containing coordinates of nodes
// 	    Examle:
// 		static int nodes[8][3] = {
// 		        0,0,0,
// 		        1,0,0,
// 		        1,1,0,
// 		        0,1,0,
// 		        0,1,1,
// 		        1,1,1,
// 		        0,1,1,
// 		        0,0,1
// 		};
//
//    2. numNodes: total number of nodes 
//
// Return: 
// 	error codes
//
RP_ERROR 
RpMesh3d::addAllNodes(int numNodes, int* nodes)
{
	if (numNodes != m_numNodes)
		return RP_ERR_WRONG_NUM_NODES;

	for (int i=0; i<numNodes; i++) {
		m_nodeList[i].set(&(nodes[i*3]));
		m_nodeList[i].id(i);
	}

	// update index
	m_nodeIndex = 0;

	return RP_SUCCESS;
}

//
// This function allows user to add one node at a time to the mesh.
//
// Input: 
// 	nodeVal int[3]
// Return: 
// 	error codes
//
RP_ERROR 
RpMesh3d::addNode(int* nodeVal)
{
	if (m_nodeIndex < m_numNodes) {
		m_nodeList[m_nodeIndex++].set(nodeVal);
		return RP_SUCCESS;
	}
	else
		return RP_ERR_TOO_MANY_NODES;
}

//
// Add an element to mesh
//
// Input:
// 	numNodes: number of nodes in element. This number must be equal 
// 		  to number of nodes/element set in the constructor 
//      nodes: array of integers as int[numNodes]
//
// Return: 
// 	error codes
//
RP_ERROR
RpMesh3d::addElement(int numNodes, const int* nodes)
{
	if (numNodes != (*m_elemList)[0].numNodes())
		return RP_ERR_WRONG_NUM_NODES;

	if (m_elemIndex < m_numElements) {
		(*m_elemList)[m_elemIndex++].addNodes(nodes, numNodes);
		return RP_SUCCESS;
	}
	else {
		m_elemIndex = 0; // reset
		return RP_ERR_TOO_MANY_ELEMENTS;
	}

}

//
// add all elements to mesh
//
// Input:
//   elemArray: a 2d array, e.g.:
// 	static int elem[6][4] = {
// 	        {0,1,5,6},
// 	        {1,2,6,7},
// 	        {2,3,7,1},
// 	        {3,4,1,2},
// 	        {5,6,2,3},
// 	        {6,7,3,4}
// 	};
//
//   numElements: number of elements in mesh
//   Note: number of nodes in an element must match the number when mesh object
//   		was instantiated.
//
//
RP_ERROR
RpMesh3d::addAllElements(int numElements, int* elemArray)
{
	if (numElements != m_numElements)
		return RP_ERR_WRONG_NUM_NODES;

	// number of nodes in an element
	int numNodes = (*m_elemList)[0].numNodes();
	for (int i=0; i < m_numElements; i++) {
		(*m_elemList)[i].addNodes(&(elemArray[i*numNodes]), numNodes);
	}

	return RP_SUCCESS;
}

// retrieve nodes 
void 
RpMesh3d::getNode(int nodeSeqNum, int* x, int* y, int* z)
{
	if (nodeSeqNum < m_numNodes)
		m_nodeList[nodeSeqNum].get(x, y, z);
}

RP_ERROR 
RpMesh3d::getNode(int nodeSeqNum, RpNode3d& node)
{
	if (nodeSeqNum >= m_numNodes)
		return RP_ERR_INVALID_INDEX;

	node = m_nodeList[nodeSeqNum];
	return RP_SUCCESS;
}

//
// Returns nodes in a list of int (nodesList)
// Input:
// 	nodesList: length should be 3*numNodes
// 	numNodes: number of nodes
// Returns:
// 	numnodes = number of nodes returned
// 	nodesList = all the nodes in mesh
//
RP_ERROR 
RpMesh3d::getNodesList(int* nodesList, int& numNodes)
{
	if (nodesList == NULL)
		return RP_ERR_NULL_PTR;
	if (numNodes > m_numNodes)
		numNodes = m_numNodes;

	int j;
	for (int i=0; i<numNodes; i++) {
		j = i * 3;
		getNode(i, &(nodesList[j]), &(nodesList[j+1]), &(nodesList[j+2]));
	}

	return RP_SUCCESS;
}

RpElement 
RpMesh3d::getElement(int elemSeqNum)
{
	return RP_SUCCESS;
}

RP_ERROR 
RpMesh3d::getElement(int elemSeqNum, int* nodesBuf)
{
	return RP_SUCCESS;
}

// serialization
char* 
RpMesh3d::serialize()
{
	return NULL;
}

RP_ERROR 
RpMesh3d::serialize(char* buf, int buflen)
{

	return RP_SUCCESS;
}

RP_ERROR 
RpMesh3d::deserialize(const char* buf)
{
	return RP_SUCCESS;
}

void 
RpMesh3d::xmlString(std::string& textString)
{
	// temp char string
	char cstr[256];
	std::string str;

	// clear the input string
	textString.erase();

	sprintf(cstr, "<mesh id=\"%s\">\n", m_id.c_str());
	textString.append(cstr);

	int i;
	for (i=0; i < m_numNodes; i++) {
		m_nodeList[i].xmlString(str);
		textString.append(str);
		str.erase();
	}

	for (i=0; i < m_numElements; i++) {
		(*m_elemList)[i].xmlString(str);
		textString.append(str);
		str.erase();
	}

	textString.append("</mesh>\n");
}

void RpMesh3d::print()
{
	string str;
	xmlString(str);
	printf("%s", str.c_str());
}

RpMesh3d::~RpMesh3d()
{
	delete [] m_elemList;
}

