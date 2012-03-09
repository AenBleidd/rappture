/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Texture1d.cpp: 1d texture class
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
#include <assert.h>
#include <math.h>

#include "Texture1D.h"
#include "Trace.h"

Texture1D::Texture1D() :
    gl_resource_allocated(false),
    id(0)
{
}

Texture1D::Texture1D(int width,
                     GLuint type, GLuint interp,
                     int numComponents, void *data) :
    gl_resource_allocated(false),
    id(0)
{
    this->width = width;
    this->type = type;
    this->interp_type = interp;
    this->n_components = numComponents;

    if (data != NULL)
        initialize(data);
}

Texture1D::~Texture1D()
{
    glDeleteTextures(1, &id);
}

GLuint Texture1D::initialize(void *data)
{
    if (gl_resource_allocated)
        glDeleteTextures(1, &id);

    glGenTextures(1, &id);

    update(data);
        
    gl_resource_allocated = true;
    return id;
}

void Texture1D::update(void *data)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBindTexture(GL_TEXTURE_1D, id);
        
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, interp_type);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, interp_type);

    GLuint format[5] = { -1, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };

    glTexImage1D(GL_TEXTURE_1D, 0, format[n_components], width, 0,
                 format[n_components], type, data);

    assert(glGetError()==0);    
}

void Texture1D::activate()
{
    glBindTexture(GL_TEXTURE_1D, id);
    glEnable(GL_TEXTURE_1D);
}

void Texture1D::deactivate()
{
    glDisable(GL_TEXTURE_1D);           
}

void Texture1D::check_max_size()
{
    GLint max = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);

    TRACE("max texture size: %d\n", max);
}

void Texture1D::check_max_unit()
{
    int max;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &max);

    TRACE("max texture units: %d.\n", max);
}
