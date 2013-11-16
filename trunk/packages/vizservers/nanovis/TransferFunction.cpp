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

#include <memory.h>
#include <assert.h>

#include <vrmath/Vector3f.h>

#include "TransferFunction.h"

using namespace nv;
using namespace vrmath;

TransferFunction::TransferFunction(const char *name, int size, float *data) :
    _name(name)
{
    // _size : # of slot, 4 : rgba
    _size = size * 4;
    _data = new float[_size];
    memcpy(_data, data, sizeof(float) * _size);

    _tex = new Texture1D(size, GL_FLOAT, GL_LINEAR, 4, data);
    _id = _tex->id();
}

TransferFunction::~TransferFunction()
{ 
    delete [] _data;
    delete _tex; 
}

void
TransferFunction::update(float *data)
{
    memcpy(_data, data, sizeof(float) * _size);
    _tex->update(_data);
}

void
TransferFunction::update(int size, float *data)
{
    // TBD..
    //assert((size*4) == _size);
    update(data);
}

void
TransferFunction::sample(float fraction, float *keys, Vector3f *keyValues, int count, Vector3f *ret)
{
    int limit = count - 1;
    if (fraction <= keys[0]) {
        *ret = keyValues[0];
    } else if (fraction >= keys[limit]) {
        *ret = keyValues[limit];
    } else {
        int n;
        for (n = 0; n < limit; n++) {
            if (fraction >= keys[n] && fraction < keys[n+1]) break;
        }
        if (n >= limit) {
            *ret = keyValues[limit];
        } else {
            float inter = (fraction - keys[n]) / (keys[n + 1] - keys[n]);
            ret->set(inter * (keyValues[n + 1].x - keyValues[n].x) + keyValues[n].x,
                     inter * (keyValues[n + 1].y - keyValues[n].y) + keyValues[n].y,
                     inter * (keyValues[n + 1].z - keyValues[n].z) + keyValues[n].z);
        }
    }
}

void
TransferFunction::sample(float fraction, float *keys, float *keyValues, int count, float *ret)
{
    int limit = count - 1;
    if (fraction <= keys[0]) {
        *ret = keyValues[0];
    } else if (fraction >= keys[limit]) {
        *ret = keyValues[limit];
    } else {
        int n;
        for (n = 0; n < limit; n++) {
            if (fraction >= keys[n] && fraction < keys[n+1]) break;
        }
        if (n >= limit) {
            *ret = keyValues[limit];
        } else {
            float inter = (fraction - keys[n]) / (keys[n + 1] - keys[n]);
            *ret = inter * (keyValues[n + 1] - keyValues[n]) + keyValues[n];
        }
    }
}
