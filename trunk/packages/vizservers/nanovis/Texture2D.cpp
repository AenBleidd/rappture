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
    gl_resource_allocated(false),
    id(0)
{}

Texture2D::Texture2D(int width, int height,
                     GLuint type, GLuint interp,
                     int numComponents, void *data) :
    gl_resource_allocated(false),
    id(0)
{
    this->width = width;
    this->height = height;
    this->type = type;
    this->interp_type = interp;
    this->n_components = numComponents;

    if (data != NULL)
        initialize(data);
}

Texture2D::~Texture2D()
{
    glDeleteTextures(1, &id);
}

GLuint Texture2D::initialize(void *data)
{
    if (gl_resource_allocated)
        glDeleteTextures(1, &id);

    glGenTextures(1, &id);

    update(data);

    gl_resource_allocated = true;
    return id;
}

void Texture2D::update(void *data)
{
    glBindTexture(GL_TEXTURE_2D, id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interp_type);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interp_type);

    //to do: add handling to more formats
#ifdef NV40
    if (type == GL_FLOAT) {
        GLuint targetFormat[5] = { -1, GL_LUMINANCE16F_ARB, GL_LUMINANCE_ALPHA16F_ARB, GL_RGB16F_ARB, GL_RGBA16F_ARB };
        GLuint format[5] = { -1, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
        glTexImage2D(GL_TEXTURE_2D, 0, targetFormat[n_components], width, height, 0, 
                     format[n_components], type, data);
    } else {
#endif
        GLuint format[5] = { -1, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
        glTexImage2D(GL_TEXTURE_2D, 0, format[n_components], width, height, 0, 
                     format[n_components], type, data);
#ifdef NV40
    }
#endif
    assert(glGetError() == 0);

    gl_resource_allocated = true;
}

void 
Texture2D::activate()
{
    glBindTexture(GL_TEXTURE_2D, id);
    glEnable(GL_TEXTURE_2D);
}

void 
Texture2D::deactivate()
{
    glDisable(GL_TEXTURE_2D);           
}

void 
Texture2D::check_max_size()
{
    GLint max = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
        
    TRACE("max texture size: %d\n", max);
}

void 
Texture2D::check_max_unit()
{
    int max = 0;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &max);

    TRACE("max texture units: %d.\n", max);
}
