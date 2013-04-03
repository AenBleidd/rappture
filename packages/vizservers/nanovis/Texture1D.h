/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Texture1D.h: 1d texture class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef TEXTURE1D_H
#define TEXTURE1D_H

#include <GL/glew.h>

namespace nv {

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

    int width() const
    {
        return _width;
    }

    GLuint id() const
    {
        return _id;
    }

    void setWrapS(GLuint wrapMode);

    static void checkMaxSize();

    static void checkMaxUnit();

private:
    int _width;

    int _numComponents;

    bool _glResourceAllocated;
    GLuint _id;
    GLuint _type;
    GLuint _interpType;
    GLuint _wrapS;
};

}

#endif
