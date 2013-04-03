/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Render to vertex array class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RENDERVERTEXARRAY_H
#define RENDERVERTEXARRAY_H

#include <GL/glew.h>
#include <GL/gl.h>

namespace nv {

class RenderVertexArray
{
public:
    RenderVertexArray(int nverts, GLint size, GLenum type = GL_FLOAT);

    ~RenderVertexArray();

    void loadData(void *data);	// load vertex data from memory

    void read(/*GLenum buffer,*/ int w, int h);   // read vertex data from
						  // frame buffer
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
