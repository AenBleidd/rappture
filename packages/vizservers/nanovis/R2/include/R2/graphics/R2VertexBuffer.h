/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef _R2_VERTEXBUFFER_H_
#define _R2_VERTEXBUFFER_H_

class R2VertexBuffer {
	unsigned int _graphicObjectID;
	int _byteSize;
	int _vertexCount;
public :
	void* _data;
public :
	enum {
		POSITION3 = 0x01,
		NORMAL3 = 0x02,
		COLOR3 = 0x04,
		COLOR4 = 0x08
	};

public :
	R2VertexBuffer(int type, int vertexCount, int byteSize, void* data, bool copy = true);
	~R2VertexBuffer();

public :
	unsigned int getGraphicObjectID() const;

	void updateBuffer(void* data);
	int getVertexCount() const;
};

inline int R2VertexBuffer::getVertexCount() const
{
	return _vertexCount;
}

inline unsigned int R2VertexBuffer::getGraphicObjectID() const
{
	return _graphicObjectID;
}

#endif //
