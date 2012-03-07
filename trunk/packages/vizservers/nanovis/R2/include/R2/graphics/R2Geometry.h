 /* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef R2_GEOMETRY_H
#define R2_GEOMETRY_H

#include <GL/glew.h>
#include <R2/graphics/R2VertexBuffer.h>
#include <R2/graphics/R2IndexBuffer.h>

class R2Geometry
{
public :
    enum {
        LINES = GL_LINES,
        LINE_STRIP = GL_LINE_STRIP,
        TRIANGLES = GL_TRIANGLES,
        TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
        QUADS = GL_QUADS
    };

public :
    R2Geometry(int primitive, R2VertexBuffer* vertexBuffer, 
               R2IndexBuffer* indexBuffer);
    R2Geometry(int primitive, R2VertexBuffer* pointBuffer, 
               R2VertexBuffer* colorBuffer, R2IndexBuffer* indexBuffer);
    ~R2Geometry();

    void render();

private :
    R2VertexBuffer *_vertexBuffer;
    R2VertexBuffer *_colorBuffer;
    R2IndexBuffer *_indexBuffer;
    int _primitiveType;
};

#endif /*_R2_GEOMETRY_H_*/
