/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 *  Render to vertex array
 *
 *  This class implements "render to vertex array" functionality
 *  using vertex and pixel buffer objects (VBO and PBO).
 *
 *  Operation:
 *  1. A buffer object is created
 *  2. The buffer object is bound to the pixel pack (destination) buffer
 *  3. glReadPixels is used to read from the frame buffer to the buffer object
 *  4. The buffer object is bound to the vertex array
 *  5. Vertex array pointers are set
 *  
 *  Usage:
 *  1. Create a floating point pbuffer
 *  2. Create a RenderVertexArray object for each vertex attribute 
 *       you want to render to
 *  3. Render vertex data to pbuffer using a fragment program (could
 *       use multiple draw buffers here)
 *  4. Call Read() method to read data from pbuffer to vertex array
 *  5. Call SetPointer() method to set vertex array pointers
 *  6. Set any other other static vertex array attribute pointers
 *  7. Render geometry as usual using glDrawArrays or glDrawElements
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
#include <stdio.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include "RenderVertexArray.h"
#include "Trace.h"

RenderVertexArray::RenderVertexArray(int nverts, GLint size, GLenum type) :
    _usage(GL_STREAM_COPY), 
    _nverts(nverts), 
    _size(size), 
    _type(type)
{
    switch (_type) {
    case GL_HALF_FLOAT_NV:
        _bytes_per_component = 2; break;
    case GL_FLOAT:
        _bytes_per_component = sizeof(float); break;
    default:
        ERROR("unsupported RenderVertexArray type\n");
        return;
    }

    // create the buffer object
    glGenBuffersARB(1, &_buffer);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, _buffer);
    glBufferDataARB(GL_PIXEL_PACK_BUFFER_EXT, 
	_nverts*_size*_bytes_per_component, 0, _usage); // undefined data
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, 0);

    // set equivalent image format
    switch(_size) {
    case 1:
        _format = GL_LUMINANCE; break;
    case 3:
        _format = GL_RGB; break;
    case 4:
        _format = GL_RGBA; break;
    default:
        ERROR("unsupported RenderVertexArray size\n");
        return;
    }
}

RenderVertexArray::~RenderVertexArray()
{
    glDeleteBuffersARB(1, &_buffer);
}

void
RenderVertexArray::LoadData(void *data)
{
    // load data to buffer object
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, _buffer);
    glBufferDataARB(GL_PIXEL_PACK_BUFFER_EXT, 
		    _nverts*_size*_bytes_per_component, data, _usage);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, 0);
}

void
RenderVertexArray::Read(/*GLenum buffer,*/ int w, int h)
{
    // bind buffer object to pixel pack buffer
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, _buffer);
    // read from frame buffer to buffer object
    //glReadBuffer(buffer); //crash
    glReadPixels(0, 0, w, h, _format, _type, 0);

    glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, 0);
}

void
RenderVertexArray::SetPointer(GLuint index)
{
    // bind buffer object to vertex array 
    glBindBufferARB(GL_ARRAY_BUFFER, _buffer);
    //glVertexAttribPointerARB(index, _size, _type, GL_FALSE, 0, 0);          //doesn't work
    glVertexPointer(_size, _type, 0, 0);

    glBindBufferARB(GL_ARRAY_BUFFER, 0);
}
