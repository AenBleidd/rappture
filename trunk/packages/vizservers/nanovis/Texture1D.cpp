/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Texture1d.cpp: 1d texture class
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

#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "Texture1D.h"
#include "Trace.h"

using namespace nv;

Texture1D::Texture1D() :
    _width(0),
    _numComponents(3),
    _glResourceAllocated(false),
    _id(0),
    _type(GL_FLOAT),
    _interpType(GL_LINEAR),
    _wrapS(GL_CLAMP_TO_EDGE)
{
}

Texture1D::Texture1D(int width,
                     GLuint type, GLuint interp,
                     int numComponents, void *data) :
    _width(width),
    _numComponents(numComponents),
    _glResourceAllocated(false),
    _id(0),
    _type(type),
    _interpType(interp),
    _wrapS(GL_CLAMP_TO_EDGE)
{
    if (data != NULL)
        initialize(data);
}

Texture1D::~Texture1D()
{
    glDeleteTextures(1, &_id);
}

GLuint Texture1D::initialize(void *data)
{
    if (_glResourceAllocated)
        glDeleteTextures(1, &_id);

    glGenTextures(1, &_id);

    update(data);
        
    _glResourceAllocated = true;
    return _id;
}

void Texture1D::update(void *data)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBindTexture(GL_TEXTURE_1D, _id);
        
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, _wrapS);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, _interpType);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, _interpType);

    GLuint format[5] = { 
        (GLuint)-1, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA
    };

    glTexImage1D(GL_TEXTURE_1D, 0, format[_numComponents], _width, 0,
                 format[_numComponents], _type, data);

    assert(glGetError()==0);    
}

void Texture1D::activate()
{
    glBindTexture(GL_TEXTURE_1D, _id);
    glEnable(GL_TEXTURE_1D);
}

void Texture1D::deactivate()
{
    glDisable(GL_TEXTURE_1D);           
}

void Texture1D::setWrapS(GLuint wrapMode)
{
    _wrapS = wrapMode;
}

void Texture1D::checkMaxSize()
{
    GLint max = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);

    TRACE("max texture size: %d", max);
}

void Texture1D::checkMaxUnit()
{
    int max;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &max);

    TRACE("max texture units: %d", max);
}
