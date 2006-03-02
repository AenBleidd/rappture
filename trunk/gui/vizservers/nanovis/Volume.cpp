/*
 * ----------------------------------------------------------------------
 * Volume.cpp: 3d volume class
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

#include "Volume.h"


Volume::Volume(int w, int h, int d, NVISdatatype t, NVISinterptype interp, int n, float* data){

  tex = new Texture3D(w, h, d, t, interp, n);
  tex->initialize(data);
  id = tex->id;

  width = w;
  height = h;
  depth = d; 

  aspect_ratio_width = tex->aspect_ratio_width;
  aspect_ratio_height = tex->aspect_ratio_height;
  aspect_ratio_depth = tex->aspect_ratio_depth;

  type = t;
  interp_type = interp;
  n_components = n;
}


Volume::~Volume(){ delete tex; }
