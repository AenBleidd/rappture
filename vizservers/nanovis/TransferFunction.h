
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

#ifndef _TRANSFER_FUNCTION_H_
#define _TRANSFER_FUNCTION_H_


#include "Texture1D.h"


class TransferFunction{
  int size;	//the resolution of the color map, how many (RGBA) quadraples
  float* data;
  Texture1D* tex; //the texture storing the colors 

public:
  GLuint id;	//OpenGL's texture identifier
  TransferFunction(int _size, float* data);
  ~TransferFunction();
  void update(float* data);
    Texture1D* getTexture();
  float* getData() { return data; }
};

inline Texture1D* TransferFunction::getTexture()
{
    return tex;
}

#endif
