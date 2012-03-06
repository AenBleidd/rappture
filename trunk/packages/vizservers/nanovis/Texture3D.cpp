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

Texture3D::Texture3D(){ id=0; gl_resource_allocated = false; }

Texture3D::Texture3D(int width, int height, int depth, GLuint type=GL_FLOAT, GLuint interp=GL_LINEAR, int components=4)
{
    assert(type == GL_UNSIGNED_BYTE || type == GL_FLOAT|| type ==GL_UNSIGNED_INT);
    assert(interp == GL_LINEAR || interp == GL_NEAREST);
        
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
    this->n_components = components;

    this->id = 0;
    gl_resource_allocated = false;
}

void Texture3D::update(float* data)
{
    //load texture with 16 bit half floating point precision if card is 6 series NV40
    //half float with linear interpolation is only supported by 6 series and up cards
    //If NV40 not defined, data is quantized to 8-bit from 32-bit.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBindTexture(GL_TEXTURE_3D, id);
    assert(id != (GLuint)-1);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    if(interp_type==GL_LINEAR){
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else{
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    //to do: add handling to more formats
    if(type==GL_FLOAT){
        switch(n_components){
#ifdef NV40
        case 1:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE16F_ARB, width, height, depth, 0, GL_LUMINANCE, GL_FLOAT, data);
            break;
        case 2:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE_ALPHA16F_ARB, width, height, depth, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, data);
            break;
        case 3:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB, width, height, depth, 0, GL_RGB, GL_FLOAT, data);
            break;
        case 4:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F_ARB, width, height, depth, 0, GL_RGBA, GL_FLOAT, data);
            break;
#else
        case 1:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE, width, height, depth, 0, GL_LUMINANCE, GL_FLOAT, data);
            break;
        case 2:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE_ALPHA, width, height, depth, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, data);
            break;
        case 3:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, width, height, depth, 0, GL_RGB, GL_FLOAT, data);
            break;
        case 4:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, width, height, depth, 0, GL_RGBA, GL_FLOAT, data);
            break;
#endif
        default:
            break;
        }
    }


    assert(glGetError()==0);
        
    gl_resource_allocated = true;

}

GLuint Texture3D::initialize(float *data)
{
    if (id != 0) glDeleteTextures(1, &id);

    //load texture with 16 bit half floating point precision if card is 6 series NV40
    //half float with linear interpolation is only supported by 6 series and up cards
    //If NV40 not defined, data is quantized to 8-bit from 32-bit.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_3D, id);
    assert(id != (GLuint)-1);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    if(interp_type==GL_LINEAR){
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else{
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    //to do: add handling to more formats
    if(type==GL_FLOAT){
        switch(n_components){
#ifdef NV40
        case 1:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE16F_ARB, width, height, depth, 0, GL_LUMINANCE, GL_FLOAT, data);
            break;
        case 2:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE_ALPHA16F_ARB, width, height, depth, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, data);
            break;
        case 3:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB, width, height, depth, 0, GL_RGB, GL_FLOAT, data);
            break;
        case 4:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F_ARB, width, height, depth, 0, GL_RGBA, GL_FLOAT, data);
            break;
#else
        case 1:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE, width, height, depth, 0, GL_LUMINANCE, GL_FLOAT, data);
            break;
        case 2:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE_ALPHA, width, height, depth, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, data);
            break;
        case 3:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, width, height, depth, 0, GL_RGB, GL_FLOAT, data);
            break;
        case 4:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, width, height, depth, 0, GL_RGBA, GL_FLOAT, data);
            break;
#endif
        default:
            break;
        }
    }


    assert(glGetError()==0);
        
    gl_resource_allocated = true;
    return id;
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

Texture3D::~Texture3D()
{
    glDeleteTextures(1, &id);
}

void Texture3D::check_max_size(){
    GLint max = 0;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_EXT, &max);
        
    //TRACE("%d", glGetError());
    TRACE("max 3d texture size: %d\n", max);
}

void Texture3D::check_max_unit(){
    int max;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &max);

    TRACE("max texture units: %d.\n", max);
}
