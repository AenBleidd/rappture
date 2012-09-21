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
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
                             int channelsPerPixel)
{
    _width = w;
    _height = h;

    //only allow two types
    assert(type == GL_FLOAT || type == GL_UNSIGNED_BYTE); 
    _dataType = type;
  
    //only allow RGB or RGBA
    assert(channelsPerPixel == 3 || channelsPerPixel == 4); 
    _numChannelsPerPixel = channelsPerPixel;

    if (type == GL_FLOAT)
        _data = new float[w*h*channelsPerPixel];
    else if (type == GL_UNSIGNED_BYTE)
        _data = new unsigned char[w*h*channelsPerPixel];
  
    assert(_data != 0);
    reset(0);   //reset data
}

ScreenSnapper::~ScreenSnapper()
{
    if (_dataType == GL_FLOAT)
        delete[] (float *)_data;
    else if(_dataType == GL_UNSIGNED_BYTE)
        delete[] (unsigned char*)_data;
}

void 
ScreenSnapper::reset(char c)
{
    unsigned int elemSize;
    switch (_dataType) {
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
    size = elemSize * _width * _height * _numChannelsPerPixel;
    memset(_data, size, c);
}

void 
ScreenSnapper::capture()
{
    if (_dataType == GL_FLOAT) {
        if (_numChannelsPerPixel == 3)
            glReadPixels(0, 0, _width, _height, GL_RGB, GL_FLOAT, _data);
        else if (_numChannelsPerPixel == 4)
            glReadPixels(0, 0, _width, _height, GL_RGBA, GL_FLOAT, _data);
    } else if (_dataType == GL_UNSIGNED_BYTE) {
        if (_numChannelsPerPixel == 3)
            glReadPixels(0, 0, _width, _height, GL_RGB, GL_UNSIGNED_BYTE, _data);
        else if (_numChannelsPerPixel == 4)
            glReadPixels(0, 0, _width, _height, GL_RGBA, GL_UNSIGNED_BYTE, _data);
    }
    assert(glGetError() == 0);
}

void
ScreenSnapper::print()
{
    for (int i = 0; i < _width*_height; i++) {
        if (_dataType == GL_FLOAT) {
            if (_numChannelsPerPixel == 3) {
                TRACE("(%f %f %f) ", ((float *)_data)[3*i], 
                      ((float*)_data)[3*i+1], ((float *)_data)[3*i+2]);
            } else if (_numChannelsPerPixel == 4) {
                TRACE("(%f %f %f %f) ", ((float *)_data)[4*i], 
                      ((float *)_data)[4*i+1],
                      ((float *)_data)[4*i+2],
                      ((float *)_data)[4*i+3]);
            }
        } else if (_dataType == GL_UNSIGNED_BYTE) {
            if (_numChannelsPerPixel == 3) {
                TRACE("(%d %d %d) ", 
                      ((unsigned char *)_data)[3*i], 
                      ((unsigned char *)_data)[3*i+1], 
                      ((unsigned char *)_data)[3*i+2]);
            } else if (_numChannelsPerPixel == 4) {
                TRACE("(%d %d %d %d) ", 
                      ((unsigned char *)_data)[4*i], 
                      ((unsigned char *)_data)[4*i+1],
                      ((unsigned char *)_data)[4*i+2],
                      ((unsigned char *)_data)[4*i+3]);
            }
        }
    }
}

