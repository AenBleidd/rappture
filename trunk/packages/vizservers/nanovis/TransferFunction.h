/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ColorMap.h: color map class contains an array of (RGBA) values 
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
#ifndef TRANSFER_FUNCTION_H
#define TRANSFER_FUNCTION_H

#include <vrmath/Vector3f.h>

#include "Texture1D.h"

class TransferFunction
{
public:
    TransferFunction(int size, float *data);

    void update(float *data);

    void update(int size, float *data);

    GLuint id()
    {
        return _id;
    }

    void id(GLuint id)
    {
        _id = id;
    }

    Texture1D *getTexture()
    {
        return _tex;
    }

    float *getData()
    { 
        return _data; 
    }

    int getSize() const
    {
        return _size;
    }

    const char *name() const
    {
        return _name;
    }

    void name(const char *name)
    {
        _name = name;
    }

    static void sample(float fraction, float *key, int count, vrmath::Vector3f *keyValue, vrmath::Vector3f *ret);

    static void sample(float fraction, float *key, int count, float *keyValue, float *ret);

protected :
    ~TransferFunction();

private:
    /// the resolution of the color map, how many (RGBA) quadraples
    int _size;
    float *_data;
    Texture1D *_tex;	///< the texture storing the colors 
    const char *_name;
    GLuint _id;		///< OpenGL texture identifier
};

#endif
