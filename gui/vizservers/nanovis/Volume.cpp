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


Volume::Volume(int x, int y, int z,
		int w, int h, int d, 
		NVISdatatype t, NVISinterptype interp, int n, float* data):
	location(Vector3(x,y,z)),
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

}


Volume::~Volume(){ delete tex; }
