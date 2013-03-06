/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 */
#include <memory.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include "VertexBuffer.h"

using namespace nv::graphics;

VertexBuffer::VertexBuffer(int type, int vertexCount,
                           int byteSize, void *data, bool copy) :
    _graphicObjectID(0),
    _byteSize(byteSize),
    _vertexCount(vertexCount)
{
    if (copy) {
        _data = (void*) malloc(byteSize);
    } else {
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

VertexBuffer::~VertexBuffer()
{
    if (_graphicObjectID != 0) {
        glDeleteBuffers(1, &_graphicObjectID);
    }
    if (_data) {
        free(_data);
    }
}
