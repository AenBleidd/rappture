/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdlib.h>
#include <memory.h>

#include <R2/graphics/R2IndexBuffer.h>

R2IndexBuffer::R2IndexBuffer(int indexCount, int* data, bool copy)
: _indexCount(indexCount)
{
	if (copy)
	{
		_data = (int*) malloc(sizeof(int) * indexCount);
		memcpy(_data, data, sizeof(int) * indexCount);
	}
	else
	{
		_data = data;
	}
}


R2IndexBuffer::~R2IndexBuffer()
{
	if (_data)
	{
		free(_data);
	}
}
