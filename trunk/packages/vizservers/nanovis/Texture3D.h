/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * texture3d.h: 3d texture class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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

    /// Always 1
    double aspectRatioWidth() const
    {
        return _aspectRatioWidth;
    }

    /// height / width
    double aspectRatioHeight() const
    {
        return _aspectRatioHeight;
    }

    /// depth / width
    double aspectRatioDepth() const
    {
        return _aspectRatioDepth;
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

    double _aspectRatioWidth;
    double _aspectRatioHeight;
    double _aspectRatioDepth;

    int _numComponents;

    bool _glResourceAllocated;
    GLuint _id;
    GLuint _type;
    GLuint _interpType;
    GLuint _wrapS;
    GLuint _wrapT;
    GLuint _wrapR;
};

#endif
