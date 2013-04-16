/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author:
 *   Wei Qiao <qiaow@purdue.edu>
 */
#ifndef NV_RENDERVERTEXARRAY_H
#define NV_RENDERVERTEXARRAY_H

#include <GL/glew.h>
#include <GL/gl.h>

namespace nv {

class RenderVertexArray
{
public:
    RenderVertexArray(int nverts, GLint size, GLenum type = GL_FLOAT);

    ~RenderVertexArray();

    void loadData(void *data);	// load vertex data from memory

    void read(int w, int h);   // read vertex data from frame buffer

    void setPointer(GLuint index);

private:
    GLenum _usage;     // vbo usage flag
    GLuint _buffer;
    GLuint _index;
    GLuint _nverts;
    GLint _size;       // size of attribute       
    GLenum _format;    // readpixels image format
    GLenum _type;      // FLOAT or HALF_FLOAT
    int _bytesPerComponent;
};

}

#endif
