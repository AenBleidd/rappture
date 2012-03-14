/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ScreenSnapper.h: ScreenSnapper class. It captures the render result
 *                      and stores it in an array of chars or floats 
 *                      depending on chosen format.
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

#include <memory.h>
#include <assert.h>

#include "ScreenSnapper.h"
#include "Trace.h"

ScreenSnapper::ScreenSnapper(int w, int h, GLuint type, 
                             int channel_per_pixel)
{
    width = w;
    height = h;

    //only allow two types
    assert(type == GL_FLOAT || type == GL_UNSIGNED_BYTE); 
    data_type = type;
  
    //only allow RGB or RGBA
    assert(channel_per_pixel == 3 || channel_per_pixel == 4); 
    n_channels_per_pixel = channel_per_pixel;

    if (type == GL_FLOAT)
        data = new float[w*h*channel_per_pixel];
    else if (type == GL_UNSIGNED_BYTE)
        data = new unsigned char[w*h*channel_per_pixel];
  
    assert(data != 0);
    reset(0);   //reset data
}

ScreenSnapper::~ScreenSnapper()
{
    if (data_type == GL_FLOAT)
        delete[] (float*)data;
    else if(data_type == GL_UNSIGNED_BYTE)
        delete[] (unsigned char*)data;
}

void 
ScreenSnapper::reset(char c)
{
    unsigned int elemSize;
    switch (data_type) {
    case GL_FLOAT:
        elemSize = sizeof(float);
        break;
    case GL_UNSIGNED_BYTE:
        elemSize = sizeof(unsigned char);
        break;
    default:
        assert(0);
        break;
    }
    unsigned int size;
    size = elemSize * width * height * n_channels_per_pixel;
    memset(data, size, c);
}

void 
ScreenSnapper::capture()
{
    if (data_type == GL_FLOAT) {
        if (n_channels_per_pixel == 3)
            glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, data);
        else if (n_channels_per_pixel == 4)
            glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, data);
    } else if (data_type == GL_UNSIGNED_BYTE) {
        if (n_channels_per_pixel == 3)
            glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
        else if (n_channels_per_pixel == 4)
            glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    assert(glGetError() == 0);
}

void
ScreenSnapper::print()
{
    for (int i = 0; i < width*height; i++) {
        if (data_type == GL_FLOAT) {
            if (n_channels_per_pixel == 3) {
                TRACE("(%f %f %f) ", ((float*)data)[3*i], 
                      ((float*)data)[3*i+1], ((float*)data)[3*i+2]);
            } else if (n_channels_per_pixel==4) {
                TRACE("(%f %f %f %f) ", ((float*)data)[4*i], 
                      ((float*)data)[4*i+1],
                      ((float*)data)[4*i+2],
                      ((float*)data)[4*i+3]);
            }
        } else if (data_type == GL_UNSIGNED_BYTE) {
            if (n_channels_per_pixel==3) {
                TRACE("(%d %d %d) ", 
                      ((unsigned char*)data)[3*i], 
                      ((unsigned char*)data)[3*i+1], 
                      ((unsigned char*)data)[3*i+2]);
            } else if (n_channels_per_pixel == 4) {
                TRACE("(%d %d %d %d) ", 
                      ((unsigned char*)data)[4*i], 
                      ((unsigned char*)data)[4*i+1],
                      ((unsigned char*)data)[4*i+2],
                      ((unsigned char*)data)[4*i+3]);
            }
        }
    }
}

