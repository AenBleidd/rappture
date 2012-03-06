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

#include "Texture1D.h"
#include "Trace.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>


Texture1D::Texture1D(){
    id = -1;			
    gl_resource_allocated = false;
}

Texture1D::Texture1D(int width, int type)
{
    assert(type == GL_UNSIGNED_BYTE || type == GL_FLOAT || 
	   type == GL_UNSIGNED_INT);
        
    this->width = width;
    this->type = type;

    id = -1;
    gl_resource_allocated = false;
}

GLuint Texture1D::initialize_float_rgba(float *data)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_1D, id);
    assert(id != (GLuint)-1);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, width, 0, GL_RGBA, GL_FLOAT, data);
    assert(glGetError()==0);
        
    gl_resource_allocated = true;
    return id;
}

void Texture1D::update_float_rgba(float* data){
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_1D, id);
        
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, width, 0, GL_RGBA, GL_FLOAT, data);
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

Texture1D::~Texture1D()
{
    glDeleteTextures(1, &id);
}

void Texture1D::check_max_size(){
    GLint max = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
        
    //TRACE("%d", glGetError());
    TRACE("max texture size: %d\n", max);
}

void Texture1D::check_max_unit(){
    int max;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &max);

    TRACE("max texture units: %d.\n", max);
}
