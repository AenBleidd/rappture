#include "element.h"

// default constructor
RpElement::RpElement()
{
	m_nodes.resize(8);
}

// constructor
// Input: 
// 	number of nodes in element
// 	element id
//
RpElement::RpElement(int numNodes, int id)
{
	m_id = id;
	if (m_nodes.capacity() != (unsigned)numNodes)
		m_nodes.resize(numNodes);
}

//
// constructor
// Instantiate from a byte stream
// id(int), numNodes(int), list of ids(int *)
//
RpElement::RpElement(const char* buf)
{
	deserialize(buf);
}

// add node name 
void RpElement::addNode(int nodeName)
{
	m_nodes.push_back(nodeName);
}

// add all the nodes to element
RP_ERROR RpElement::addNodes(const int* nodes, int numNodes)
{
	// expand array if not big enough
	if (m_nodes.capacity() < (unsigned)numNodes)
		m_nodes.resize(numNodes);

	for (int i=0; i < numNodes; i++) {
		m_nodes[i] = nodes[i];
	}

	return RP_SUCCESS;
}

// 
// Return a list of node IDs 
// Input: 
// 	inlist: int array
// 	len:    length of inlist
// Output:
// 	inlist: IDs of nodes in this element
// 	len: number of nodes in element
//
RP_ERROR RpElement::getNodeIdList(int* inlist, int& len)
{
	int size = m_nodes.size();

	if (inlist == NULL || len < size) {
		RpAppendErr("RpElement::getNodeIdList: invalid array");
		RpPrintErr();
		return RP_ERR_INVALID_ARRAY;
	}

	for (int i=0; i < size; i++) {
		inlist[i] = m_nodes[i];
	}

	len = size;

	return RP_SUCCESS;
}

//
// Serialization of RpElement:
// 	element id
// 	number of nodes in element
// 	node 1
// 	node 2
// 	...
//
char* 
RpElement::serialize()
{
	int nbytes = this->numBytes() + 2*sizeof(int);

	char * buf = new char[nbytes];
	serialize(buf, nbytes);
	return buf;
}

RP_ERROR
RpElement::serialize(char* buf, int buflen)
{
	int nbytes = this->numBytes();

	if (buf == NULL || buflen < (signed)(nbytes+2*sizeof(int))) {
		RpAppendErr("RpNode::serialize: invalid buffer");
		RpPrintErr();
		return RP_ERR_INVALID_ARRAY;
	}

	char * ptr = buf;

	// copy element id
	memcpy((void *)ptr, (void *)&m_id, sizeof(int));
	ptr += sizeof(int);

	// copy number of nodes/element
	int numNodes = m_nodes.size();
	memcpy((void *)ptr, (void *)&numNodes, sizeof(int));
	ptr += sizeof(int);

	// copy all nodes
	memcpy((void *)ptr, (void *)&(m_nodes[0]), numNodes*sizeof(int));

	return RP_SUCCESS;
}

RP_ERROR
RpElement::deserialize(const char* buf)
{
	if (buf == NULL) {
		RpAppendErr("RpElement::deserialize: null buffer");
                RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	char * ptr = (char*)buf;

	// copy element id
	memcpy((void *)ptr, (void *)&m_id, sizeof(int));
	ptr += sizeof(int);

	// copy number of nodes
	int numNodes;
	memcpy((void *)ptr, (void *)&numNodes, sizeof(int));

	if (m_nodes.capacity() != (unsigned)numNodes)
		m_nodes.resize(numNodes);

	for (int i=0; i < numNodes; i++) {
		memcpy((void *)&(m_nodes[i]), (void *)ptr, sizeof(int));
		ptr += sizeof(int);
	}
	return RP_SUCCESS;
}

void
RpElement::xmlString(std::string& textString)
{
	// temp char string
	char str[256];

	// clear the input string
	textString.erase();

	sprintf(str, "<element id=\"%d\">\n\t<nodes>", m_id);
	textString.append(str);

	for (int i=0; i < (signed)(m_nodes.size()); i++) {
		sprintf(str, "%d ", m_nodes[i]);
		textString.append(str);
	}
	textString.append("</nodes>\n</element>\n");
}

void 
RpElement::print()
{
	string str; 
	xmlString(str); 
	printf("%s", str.c_str());
}

