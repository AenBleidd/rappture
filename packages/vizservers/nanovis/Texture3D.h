/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * texture3d.h: 3d texture class
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
#ifndef NV_TEXTURE3D_H
#define NV_TEXTURE3D_H

#include <GL/glew.h>

namespace nv {

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

    int width() const
    {
        return _width;
    }

    int height() const
    {
        return _width;
    }

    int depth() const
    {
        return _width;
    }

    GLuint id() const
    {
        return _id;
    }

    void setWrapS(GLuint wrapMode);

    void setWrapT(GLuint wrapMode);

    void setWrapR(GLuint wrapMode);

    static void checkMaxSize();

    static void checkMaxUnit();

private:
    int _width;
    int _height;
    int _depth;

    int _numComponents;

    bool _glResourceAllocated;
    GLuint _id;
    GLuint _type;
    GLuint _interpType;
    GLuint _wrapS;
    GLuint _wrapT;
    GLuint _wrapR;
};

}

#endif
