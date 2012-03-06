/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <GL/glew.h>
#include <R2/graphics/R2VertexBuffer.h>
#include <GL/gl.h>
#include <memory.h>
#include <stdlib.h>

R2VertexBuffer::R2VertexBuffer(int type, int vertexCount, int byteSize, void* data, bool copy)
: _graphicObjectID(0), _byteSize(byteSize), _vertexCount(vertexCount)
{
	if (copy)
	{
		_data = (void*) malloc(byteSize);
	}
	else 
	{
		_data = data;
	}

	glGenBuffers(1, &_graphicObjectID);
	glBindBuffer(GL_ARRAY_BUFFER, _graphicObjectID);
    glBufferData(GL_ARRAY_BUFFER,
                  _byteSize,
                  data,
                  GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

R2VertexBuffer::~R2VertexBuffer()
{
	if (_graphicObjectID != 0)
	{
		glDeleteBuffers(1, &_graphicObjectID);
	}

	if (_data)
	{
		free(_data);
	}
}
