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
#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include <GL/glew.h>

class Texture2D
{
public:
    Texture2D();

    Texture2D(int width, int height,
              GLuint type = GL_FLOAT,
              GLuint interp = GL_LINEAR,
              int numComponents = 4,
              void *data = NULL);

    ~Texture2D();

    GLuint initialize(void *data);

    void update(void *data);

    void activate();

    void deactivate();

    static void check_max_size();

    static void check_max_unit();

    int width;
    int height;

    int n_components;

    bool gl_resource_allocated;
    GLuint id;
    GLuint type;
    GLuint interp_type;
};

#endif
