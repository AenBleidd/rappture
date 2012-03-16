/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Texture2D.h: 2d texture class
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

#include "Texture2D.h"
#include "Trace.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "config.h"

Texture2D::Texture2D() :
    _width(0),
    _height(0),
    _numComponents(3),
    _glResourceAllocated(false),
    _id(0),
    _type(GL_FLOAT),
    _interpType(GL_LINEAR),
    _wrapS(GL_CLAMP_TO_EDGE),
    _wrapT(GL_CLAMP_TO_EDGE)
{}

Texture2D::Texture2D(int width, int height,
                     GLuint type, GLuint interp,
                     int numComponents, void *data) :
    _width(width),
    _height(height),
    _numComponents(numComponents),
    _glResourceAllocated(false),
    _id(0),
    _type(type),
    _interpType(interp),
    _wrapS(GL_CLAMP_TO_EDGE),
    _wrapT(GL_CLAMP_TO_EDGE)
{
    if (data != NULL)
        initialize(data);
}

Texture2D::~Texture2D()
{
    glDeleteTextures(1, &_id);
}

GLuint Texture2D::initialize(void *data)
{
    if (_glResourceAllocated)
        glDeleteTextures(1, &_id);

    glGenTextures(1, &_id);

    update(data);

    _glResourceAllocated = true;
    return _id;
}

void Texture2D::update(void *data)
{
    glBindTexture(GL_TEXTURE_2D, _id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _wrapT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _interpType);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _interpType);

    //to do: add handling to more formats
#ifdef NV40
    if (_type == GL_FLOAT) {
        GLuint targetFormat[5] = { -1, GL_LUMINANCE16F_ARB, GL_LUMINANCE_ALPHA16F_ARB, GL_RGB16F_ARB, GL_RGBA16F_ARB };
        GLuint format[5] = { -1, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
        glTexImage2D(GL_TEXTURE_2D, 0, targetFormat[_numComponents], _width, _height, 0, 
                     format[_numComponents], _type, data);
    } else {
#endif
        GLuint format[5] = { -1, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
        glTexImage2D(GL_TEXTURE_2D, 0, format[_numComponents], _width, _height, 0, 
                     format[_numComponents], _type, data);
#ifdef NV40
    }
#endif
    assert(glGetError() == 0);
}

void Texture2D::activate()
{
    glBindTexture(GL_TEXTURE_2D, _id);
    glEnable(GL_TEXTURE_2D);
}

void Texture2D::deactivate()
{
    glDisable(GL_TEXTURE_2D);           
}

void Texture2D::setWrapS(GLuint wrapMode)
{
    _wrapS = wrapMode;
}

void Texture2D::setWrapT(GLuint wrapMode)
{
    _wrapT = wrapMode;
}

void Texture2D::check_max_size()
{
    GLint max = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
        
    TRACE("max texture size: %d\n", max);
}

void Texture2D::check_max_unit()
{
    int max = 0;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &max);

    TRACE("max texture units: %d.\n", max);
}
