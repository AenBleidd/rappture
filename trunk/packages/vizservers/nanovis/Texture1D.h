/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Texture1D.h: 1d texture class
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
#ifndef _TEXTURE_1D_H_
#define _TEXTURE_1D_H_

#include <GL/glew.h>

class Texture1D {
public:
    int width;
    bool gl_resource_allocated;
    
    GLuint type;
    GLuint id;
    GLuint tex_unit;

    Texture1D();
    Texture1D(int length, int type = GL_UNSIGNED_BYTE);
    ~Texture1D();
        
    void activate();
    void deactivate();
    GLuint initialize_float_rgba(float* data);
    void update_float_rgba(float* data);
    static void check_max_size();
    static void check_max_unit();
};

#endif
