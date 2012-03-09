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
#ifndef TEXTURE1D_H
#define TEXTURE1D_H

#include <GL/glew.h>

class Texture1D
{
public:
    Texture1D();

    Texture1D(int width,
              GLuint type = GL_FLOAT,
              GLuint interp = GL_LINEAR,
              int numComponents = 4,
              void *data = NULL);

    ~Texture1D();

    GLuint initialize(void *data);

    void update(void *data);

    void activate();

    void deactivate();

    static void check_max_size();

    static void check_max_unit();

    int width;

    int n_components;

    bool gl_resource_allocated;
    GLuint id;
    GLuint type;
    GLuint interp_type;

    //GLuint tex_unit;
};

#endif
