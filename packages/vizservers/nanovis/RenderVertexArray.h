/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Render to vertex array class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RENDERVERTEXARRAY_H
#define RENDERVERTEXARRAY_H

#include <GL/glew.h>
#include <GL/gl.h>

class RenderVertexArray {
    GLenum m_usage;     // vbo usage flag
    GLuint m_buffer;
    GLuint m_index;
    GLuint m_nverts;
    GLint m_size;       // size of attribute       
    GLenum m_format;    // readpixels image format
    GLenum m_type;      // FLOAT or HALF_FLOAT
    int m_bytes_per_component;

public:
    RenderVertexArray(int nverts, GLint size, GLenum type = GL_FLOAT);
    ~RenderVertexArray();

    void LoadData(void *data);	// load vertex data from memory
    void Read(/*GLenum buffer,*/ int w, int h);   // read vertex data from
						  // frame buffer
    void SetPointer(GLuint index);
};

#endif
