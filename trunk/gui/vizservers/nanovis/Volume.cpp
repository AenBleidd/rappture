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
	n_components(n),
	enabled(true)
{

  tex = new Texture3D(w, h, d, t, interp, n);
  tex->initialize(data);
  id = tex->id;

  aspect_ratio_width = tex->aspect_ratio_width;
  aspect_ratio_height = tex->aspect_ratio_height;
  aspect_ratio_depth = tex->aspect_ratio_depth;

  location = Vector3(x,y,z);

  //Add cut planes. We create 3 default cut planes facing x, y, z directions.
  //The default location of cut plane is in the middle of the data.
  plane.clear();
  plane.push_back(CutPlane(1, 0.5));
  plane.push_back(CutPlane(2, 0.5));
  plane.push_back(CutPlane(3, 0.5));
  
}


Volume::~Volume(){ delete tex; }


void Volume::enable() { enabled = true; }
void Volume::disable() { enabled = false; }
void Volume::move(Vector3 _loc) { location = _loc; }
bool Volume::is_enabled() { return enabled; }
Vector3* Volume::get_location() { return &location; }


int Volume::add_cutplane(int _orientation, float _location){
  plane.push_back(CutPlane(1, 0.5));
  return plane.size() - 1;
}

void Volume::enable_cutplane(int index){ 
  assert(index < plane.size());
  plane[index].enabled = true;
}

void Volume::disable_cutplane(int index){
  assert(index < plane.size());
  plane[index].enabled = false;
}

void Volume::move_cutplane(int index, float location){
  assert(index < plane.size());
  plane[index].offset = location;
}

CutPlane* Volume::get_cutplane(int index){
  assert(index < plane.size());
  return &plane[index];
}

int Volume::get_cutplane_count(){
  return plane.size();
}

bool Volume::cutplane_is_enabled(int index){
  assert(index < plane.size());
  return plane[index].enabled; 
}
