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
    gl_resource_allocated(false),
    id(0)
{}

Texture3D::Texture3D(int width, int height, int depth,
                     GLuint type, GLuint interp,
                     int numComponents, void *data) :
    gl_resource_allocated(false),
    id(0)
{
    this->width = width;
    this->height = height;
    this->depth = depth;

    //int m = (width > height) ? width : height;
    //m = (m > depth) ? m : depth; 

    //int m = max(max(width, height), depth);
    this->aspect_ratio_width = 1.;
    this->aspect_ratio_height = (double)height/(double)width;
    this->aspect_ratio_depth = (double)depth/(double)width;

    //this->aspect_ratio_width = (double)width/(double)m;
    //this->aspect_ratio_height = (double)height/(double)m;
    //this->aspect_ratio_depth = (double)depth/(double)m;

    this->type = type;
    this->interp_type = interp;
    this->n_components = numComponents;

    if (data != NULL)
        initialize(data);
}

Texture3D::~Texture3D()
{
    glDeleteTextures(1, &id);
}

GLuint Texture3D::initialize(void *data)
{
    if (id != 0)
        glDeleteTextures(1, &id);

    glGenTextures(1, &id);

    update(data);
 
    return id;
}

void Texture3D::update(void *data)
{
    assert(id > 0 && id != (GLuint)-1);
    glBindTexture(GL_TEXTURE_3D, id);

    //load texture with 16 bit half floating point precision if card is 6 series NV40
    //half float with linear interpolation is only supported by 6 series and up cards
    //If NV40 not defined, data is quantized to 8-bit from 32-bit.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, interp_type);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, interp_type);

    //to do: add handling to more formats
#ifdef NV40
    if (type == GL_FLOAT) {
        GLuint targetFormat[5] = { -1, GL_LUMINANCE16F_ARB, GL_LUMINANCE_ALPHA16F_ARB, GL_RGB16F_ARB, GL_RGBA16F_ARB };
        GLuint format[5] = { -1, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
        glTexImage3D(GL_TEXTURE_3D, 0, targetFormat[n_components],
                     width, height, depth, 0, 
                     format[n_components], type, data);
    } else {
#endif
        GLuint format[5] = { -1, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
        glTexImage3D(GL_TEXTURE_3D, 0, format[n_components],
                     width, height, depth, 0, 
                     format[n_components], type, data);
#ifdef NV40
    }
#endif

    assert(glGetError()==0);

    gl_resource_allocated = true;
}

void Texture3D::activate()
{
    glEnable(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, id);
}

void Texture3D::deactivate()
{
    glDisable(GL_TEXTURE_3D);           
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
