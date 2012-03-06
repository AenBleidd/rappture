/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ColorMap.h: color map class contains an array of (RGBA) values 
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


#include "TransferFunction.h"
#include <memory.h>
#include <assert.h>

TransferFunction::TransferFunction(int size, float *data)
{
    _tex = new Texture1D(size, GL_FLOAT);

    // _size : # of slot, 4 : rgba
    _size = size * 4;
    _data = new float[_size];
    memcpy(_data, data, sizeof(float) * _size);
    _tex->initialize_float_rgba(_data);
    _id = _tex->id;
}


TransferFunction::~TransferFunction()
{ 
    delete [] _data;
    delete _tex; 
}

void 
TransferFunction::update(float* data)
{
    memcpy(_data, data, sizeof(float) * _size);
    _tex->update_float_rgba(_data);
}


void 
TransferFunction::update(int size, float *data)
{
    // TBD..
    //assert((size*4) == _size);
    update(data);
}

