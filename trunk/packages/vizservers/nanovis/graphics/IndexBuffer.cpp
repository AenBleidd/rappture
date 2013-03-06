/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 */
#include <stdlib.h>
#include <memory.h>

#include "IndexBuffer.h"

using namespace nv::graphics;

IndexBuffer::IndexBuffer(int indexCount, int* data, bool copy) :
    _indexCount(indexCount)
{
    if (copy) {
        _data = (int*)malloc(sizeof(int) * indexCount);
        memcpy(_data, data, sizeof(int) * indexCount);
    } else {
        _data = data;
    }
}

IndexBuffer::~IndexBuffer()
{
    if (_data) {
        free(_data);
    }
}
