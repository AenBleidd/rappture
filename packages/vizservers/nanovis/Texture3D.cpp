/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Texture3d.cpp: 3d texture class
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

#include "Texture3D.h"
#include "Trace.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "config.h"

Texture3D::Texture3D() :
    _width(0),
    _height(0),
    _depth(0),
    _numComponents(3),
    _glResourceAllocated(false),
    _id(0),
    _type(GL_FLOAT),
    _interpType(GL_LINEAR),
    _wrapS(GL_CLAMP_TO_EDGE),
    _wrapT(GL_CLAMP_TO_EDGE),
    _wrapR(GL_CLAMP_TO_EDGE)
{}

Texture3D::Texture3D(int width, int height, int depth,
                     GLuint type, GLuint interp,
                     int numComponents, void *data) :
    _width(width),
    _height(height),
    _depth(depth),
    _numComponents(numComponents),
    _glResourceAllocated(false),
    _id(0),
    _type(type),
    _interpType(interp),
    _wrapS(GL_CLAMP_TO_EDGE),
    _wrapT(GL_CLAMP_TO_EDGE),
    _wrapR(GL_CLAMP_TO_EDGE)
{
    //int m = (_width > _height) ? _width : _height;
    //m = (m > _depth) ? m : _depth; 

    //int m = max(max(_width, _height), _depth);
    _aspectRatioWidth = 1.;
    _aspectRatioHeight = (double)_height/(double)_width;
    _aspectRatioDepth = (double)_depth/(double)_width;

    //_aspectRatioWidth = (double)_width/(double)m;
    //_aspectRatioHeight = (double)_height/(double)m;
    //_aspectRatioDepth = (double)_depth/(double)m;

    if (data != NULL)
        initialize(data);
}

Texture3D::~Texture3D()
{
    glDeleteTextures(1, &_id);
}

GLuint Texture3D::initialize(void *data)
{
    if (_glResourceAllocated)
        glDeleteTextures(1, &_id);

    glGenTextures(1, &_id);

    update(data);

    _glResourceAllocated = true;
    return _id;
}

void Texture3D::update(void *data)
{
    glBindTexture(GL_TEXTURE_3D, _id);

    //load texture with 16 bit half floating point precision if card is 6 series NV40
    //half float with linear interpolation is only supported by 6 series and up cards
    //If NV40 not defined, data is quantized to 8-bit from 32-bit.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, _wrapS);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, _wrapT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, _wrapR);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, _interpType);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, _interpType);

    //to do: add handling to more formats
#ifdef NV40
    if (_type == GL_FLOAT) {
        GLuint targetFormat[5] = { -1, GL_LUMINANCE16F_ARB, GL_LUMINANCE_ALPHA16F_ARB, GL_RGB16F_ARB, GL_RGBA16F_ARB };
        GLuint format[5] = { -1, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
        glTexImage3D(GL_TEXTURE_3D, 0, targetFormat[_numComponents],
                     _width, _height, _depth, 0, 
                     format[_numComponents], _type, data);
    } else {
#endif
        GLuint format[5] = { -1, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
        glTexImage3D(GL_TEXTURE_3D, 0, format[_numComponents],
                     _width, _height, _depth, 0, 
                     format[_numComponents], _type, data);
#ifdef NV40
    }
#endif

    assert(glGetError()==0);
}

void Texture3D::activate()
{
    glEnable(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, _id);
}

void Texture3D::deactivate()
{
    glDisable(GL_TEXTURE_3D);           
}

void Texture3D::setWrapS(GLuint wrapMode)
{
    _wrapS = wrapMode;
}

void Texture3D::setWrapT(GLuint wrapMode)
{
    _wrapT = wrapMode;
}

void Texture3D::setWrapR(GLuint wrapMode)
{
    _wrapR = wrapMode;
}

void Texture3D::check_max_size()
{
    GLint max = 0;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_EXT, &max);

    TRACE("max 3d texture size: %d\n", max);
}

void Texture3D::check_max_unit()
{
    int max;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &max);

    TRACE("max texture units: %d.\n", max);
}
