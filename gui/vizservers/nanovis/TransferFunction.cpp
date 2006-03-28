
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


TransferFunction::TransferFunction(int _size, float* data){

  tex = new Texture1D(_size, GL_FLOAT);

  tex->initialize_float_rgba(data);
  id = tex->id;
}


TransferFunction::~TransferFunction(){ delete tex; }

void TransferFunction::update(float* data){
  tex->update_float_rgba(data);
}


