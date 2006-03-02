
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

#ifndef _COLOR_MAP_H_
#define _COLOR_MAP_H_


#include "Texture1D.h"


class ColorMap{
  int size;	//the resolution of the color map, how many (RGBA) quadraples
  Texture1D* tex; //the texture storing the colors 

  GLuint id;	//OpenGL's texture identifier


public:
  ColorMap(int _size, float* data);
  ~ColorMap();
};


#endif
