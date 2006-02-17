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

void
RpNode::setNode2d(int xval, int yval, const char* id)
{
	m_node.resize(2);

	m_dim = 2;
	m_node[0] = xval;
	m_node[1] = yval;
	m_id = id;
}

void
RpNode::setNode3d(int xval, int yval, int zval, const char* id)
{
	m_node.resize(3);

	m_dim = 3;
	m_node[0] = xval;
	m_node[1] = yval;
	m_node[2] = zval;
	m_id = id;
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
