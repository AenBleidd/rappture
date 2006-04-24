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


Volume::Volume(float x, float y, float z,
		int w, int h, int d, 
		NVISdatatype t, NVISinterptype interp, int n, float* data):
	width(w),
	height(h),
	depth(d),
	type(t),
	interp_type(interp),
	n_components(n)
{

  tex = new Texture3D(w, h, d, t, interp, n);
  tex->initialize(data);
  id = tex->id;

  aspect_ratio_width = tex->aspect_ratio_width;
  aspect_ratio_height = tex->aspect_ratio_height;
  aspect_ratio_depth = tex->aspect_ratio_depth;

  location = Vector3(x,y,z);

  //add cut planes
  plane.clear();
  plane.push_back(CutPlane(1, 0.5));
  plane.push_back(CutPlane(2, 0.5));
  plane.push_back(CutPlane(3, 0.5));
  
}


Volume::~Volume(){ delete tex; }
