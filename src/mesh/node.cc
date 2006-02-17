#include "node.h"

//
// assign value to coordinate[index]
//
RP_ERROR 
RpNode::set(int val, int index)
{
	RP_ERROR err = RP_SUCCESS;

	if (index < m_dim) 
		m_node[index] = val;
	else {
		RpAppendErr("RpNode::set: index out of bound");
		RpPrintErr();
		err = RP_ERR_OUTOFBOUND_INDEX;
	}

	return err;
}

// 
// set values in node "id"
//
void
RpNode::setNode(int dim, int* val, int id)
{
	m_dim = dim;
	m_id = id;
	m_node.resize(m_dim);

	for (int i=0; i<m_dim; i++) {
		m_node[i] = val[i];
	}
}

// 
// get node values 
// Input: 
// 	node: array of int for storing coordinates
// 	len:  length of 'node' array
//
RP_ERROR
RpNode::get(int* node, int len)
{
	RP_ERROR err = RP_SUCCESS;

	if (node != NULL && len >= m_dim)
		for (int i=0; i<m_dim; i++) {
			node[i] = m_node[i];
		}
	else
		err = RP_ERR_INVALID_ARRAY;

	return err;
}

RP_ERROR 
RpNode::get(int& val, int index)
{
	RP_ERROR err = RP_SUCCESS;

	if (index < m_dim) 
		val =  m_node[index];
	else {
		RpAppendErr("RpNode::set: index out of bound");
		RpPrintErr();
		err = RP_ERR_OUTOFBOUND_INDEX;
	}

	return err;
}

char* 
RpNode::serialize()
{
	int nbytes = (m_node.size()) * sizeof(int);

	// total length of streams buffer
	char * buf = new char[nbytes];
	if (buf == NULL) {
		RpAppendErr("RpNode::serialize: mem alloc failed");
		RpPrintErr();
		return buf;
	}

	serialize(buf, nbytes);

	return buf;
}

//
// serialize with an input streams buffer
// 4 bytes: x
// 4 bytes: y
// 4 bytes: z (if 3d node)
//
void
RpNode::serialize(const char* buf, int buflen)
{
	char* ptr = (char*)buf;

	// do nothing
	if (buf == NULL || buflen < m_dim* ((signed int)sizeof(int))) {
		RpAppendErr("RpNode::serialize: buffer not valid");
		RpPrintErr();
		return;
	}

	// write to stream buffer
	for (int i=0; i<m_dim; i++) {
		memcpy((void*)ptr, (void*)&(m_node[i]), sizeof(int));
		ptr += sizeof(int);
	}
}

void 
RpNode::deserialize(const char* buf)
{
	if (buf == NULL) {
		RpAppendErr("RpNode::serialize: null buffer");
		RpPrintErr();
		return;
	}

	// write to stream buffer
	int i = 0;
	while (i < m_dim && buf) {
		memcpy((void*)&(m_node[i]), (void*)buf, sizeof(int));
		buf += sizeof(int);
		i++;
	}

	return;
}

void RpNode::print()
{
	for (int i=0; i<m_dim; i++)
		printf("%d ", m_node[i]);
	printf("\n");
}
