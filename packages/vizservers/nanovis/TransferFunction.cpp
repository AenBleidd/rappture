/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ColorMap.h: color map class contains an array of (RGBA) values 
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

#include "TransferFunction.h"
#include <memory.h>
#include <assert.h>

TransferFunction::TransferFunction(int size, float *data)
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
TransferFunction::sample(float fraction, float *key, int count, Vector3 *keyValue, Vector3 *ret)
{
    int limit = count - 1;
    if (fraction <= key[0]) {
        *ret = keyValue[0];
    } else if (fraction >= key[limit]) {
        *ret = keyValue[limit];
    } else {
        int n;
        for (n = 0; n < limit; n++){
            if (fraction >= key[n] && fraction < key[n+1]) break;
        }
        if (n >= limit) {
            *ret = keyValue[limit];
        } else {
            float inter = (fraction - key[n]) / (key[n + 1] - key[n]);
            ret->set(inter * (keyValue[n + 1].x - keyValue[n].x) + keyValue[n].x,
                     inter * (keyValue[n + 1].y - keyValue[n].y) + keyValue[n].y,
                     inter * (keyValue[n + 1].z - keyValue[n].z) + keyValue[n].z);
        }
    }
}

void
TransferFunction::sample(float fraction, float *key, int count, float *keyValue, float *ret)
{
    int limit = count - 1;
    if (fraction <= key[0]) {
        *ret = keyValue[0];
    } else if (fraction >= key[limit]) {
        *ret = keyValue[limit];
    } else {
        int n;
        for (n = 0; n < limit; n++){
            if (fraction >= key[n] && fraction < key[n+1]) break;
        }
        if (n >= limit) {
            *ret = keyValue[limit];
        } else {
            float inter = (fraction - key[n]) / (key[n + 1] - key[n]);
            *ret = inter * (keyValue[n + 1] - keyValue[n]) + keyValue[n];
        }
    }
}
