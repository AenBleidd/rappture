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

Texture2D::Texture2D(){}

Texture2D::Texture2D(int width, int height, GLuint type=GL_FLOAT, 
	GLuint interp=GL_LINEAR, int n=4, float* data = 0)
{
    assert(type == GL_UNSIGNED_BYTE || 
	   type == GL_FLOAT || 
	   type ==GL_UNSIGNED_INT);
    assert(interp == GL_LINEAR || interp == GL_NEAREST);
        
    this->width = width;
    this->height = height;
    this->type = type;
    this->interp_type = interp;
    this->n_components = n;

    this->id = 0; 

    if(data != 0)
        initialize(data);
}

GLuint 
Texture2D::initialize(float *data)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    assert(id != (GLuint)-1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
    if(interp_type == GL_LINEAR){
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    //to do: add handling to more formats
    if (type==GL_FLOAT) {
        switch(n_components){
#ifdef NV40
        case 1:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE16F_ARB, width, height,
			 0, GL_LUMINANCE, GL_FLOAT, data);
            break;
        case 2:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA16F_ARB, width, 
			 height, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, data);
            break;
        case 3:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, width, height, 0, 
			 GL_RGB, GL_FLOAT, data);
            break;
        case 4:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, width, height, 0, 
			 GL_RGBA, GL_FLOAT, data);
            break;
#else
        case 1:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, 
			 GL_LUMINANCE, GL_FLOAT, data);
            break;
        case 2:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, width, height, 
			 0, GL_LUMINANCE_ALPHA, GL_FLOAT, data);
            break;
        case 3:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, 
			 GL_FLOAT, data);
            break;
        case 4:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, 
			 GL_FLOAT, data);
            break;
#endif
        default:
            break;
        }
    } else {
	int comp[5] = { -1, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
	glTexImage2D(GL_TEXTURE_2D, 0, comp[n_components], width, height, 0, 
		     GL_RGBA, type, data);
    }
    assert(glGetError()==0);
    return id;
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


Texture2D::~Texture2D()
{
    glDeleteTextures(1, &id);
}


void 
Texture2D::check_max_size(){
    GLint max = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
        
    TRACE("max texture size: %d\n", max);
}

void 
Texture2D::check_max_unit(){
    int max;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &max);

    TRACE("max texture units: %d.\n", max);
}

