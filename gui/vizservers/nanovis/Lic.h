/*
 * ----------------------------------------------------------------------
 * Lic.h: line integral convolution class
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


#ifndef _LIC_H_
#define _LIC_H_

#include "GL/glew.h"
#include "Cg/cgGL.h"

#include "define.h"
#include "config.h"
#include "global.h"

#include "Vector3.h"
#include "Volume.h"


class Lic{

private:
  int display_width, display_height;
  int size; 	//the lic is a square of size, it can be stretched
  float ratio_width_to_height;

  NVISid color_tex, pattern_tex, mag_tex;
  NVISid fbo, vel_fbo, slice_vector_tex;  //for projecting 3d vector to 2d vector on a plane

  Volume* vector_field; 

public:
  Vector3 normal;
  Lic(int size);
  ~Lic();

  void convolve();
  void display();
  
  

};


