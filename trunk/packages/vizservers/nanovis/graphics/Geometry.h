/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 */
#ifndef NV_GRAPHICS_GEOMETRY_H
#define NV_GRAPHICS_GEOMETRY_H

#include <GL/glew.h>

#include <graphics/VertexBuffer.h>
#include <graphics/IndexBuffer.h>

namespace nv {
namespace graphics {

class Geometry
{
public:
    enum {
        LINES = GL_LINES,
        LINE_STRIP = GL_LINE_STRIP,
        TRIANGLES = GL_TRIANGLES,
        TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
        QUADS = GL_QUADS
    };

    Geometry(int primitive, VertexBuffer *vertexBuffer, 
             IndexBuffer *indexBuffer);
    Geometry(int primitive, VertexBuffer *pointBuffer, 
             VertexBuffer *colorBuffer, IndexBuffer *indexBuffer);
    ~Geometry();

    void render();

private:
    VertexBuffer *_vertexBuffer;
    VertexBuffer *_colorBuffer;
    IndexBuffer *_indexBuffer;
    int _primitiveType;
};

}
}

#endif
