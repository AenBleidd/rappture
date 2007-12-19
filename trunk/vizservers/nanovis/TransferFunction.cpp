
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


TransferFunction::TransferFunction(int _size, float* data)
{
    tex = new Texture1D(_size, GL_FLOAT);

    // _size : # of slot, 4 : rgba
    size = _size * 4;
    this->data = new float[size];
    memcpy(this->data, data, sizeof(float) * size);

    tex->initialize_float_rgba(data);
    id = tex->id;
}


TransferFunction::~TransferFunction()
{ 
    delete [] data;
    delete tex; 
}

void TransferFunction::update(float* data)
{
    memcpy(this->data, data, sizeof(float) * size);

    tex->update_float_rgba(data);
}


