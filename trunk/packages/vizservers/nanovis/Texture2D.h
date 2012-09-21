/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Texture2D.h: 2d texture class
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

    int width() const
    {
        return _width;
    }

    int height() const
    {
        return _width;
    }

    GLuint id() const
    {
        return _id;
    }

    void setWrapS(GLuint wrapMode);

    void setWrapT(GLuint wrapMode);

    static void checkMaxSize();

    static void checkMaxUnit();

private:
    int _width;
    int _height;

    int _numComponents;

    bool _glResourceAllocated;
    GLuint _id;
    GLuint _type;
    GLuint _interpType;
    GLuint _wrapS;
    GLuint _wrapT;
};

#endif
