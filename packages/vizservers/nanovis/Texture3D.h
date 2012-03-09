/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * texture3d.h: 3d texture class
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
#ifndef TEXTURE3D_H
#define TEXTURE3D_H

#include <GL/glew.h>

class Texture3D
{
public:
    Texture3D();

    Texture3D(int width, int height, int depth,
              GLuint type = GL_FLOAT,
              GLuint interp = GL_LINEAR,
              int numComponents = 4,
              void *data = NULL);

    ~Texture3D();

    GLuint initialize(void *data);

    void update(void *data);

    void activate();

    void deactivate();

    static void check_max_size();

    static void check_max_unit();

    int width;
    int height;
    int depth;

    double aspect_ratio_width;
    double aspect_ratio_height;
    double aspect_ratio_depth;

    int n_components;

    bool gl_resource_allocated;
    GLuint id;
    GLuint type;
    GLuint interp_type;
    //GLuint tex_unit;
};

#endif
