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

#include <assert.h>
#include "Volume.h"


Volume::Volume(float x, float y, float z,
		int w, int h, int d, float s, 
		int n, float* data):
	width(w),
	height(h),
	depth(d),
	size(s),
	n_components(n),
	enabled(true),
	n_slice(256), // default value
	specular(6.), // default value
	diffuse(3.), // default value
	opacity_scale(10.), // default value
	data_enabled(true), // default value
	outline_enabled(true), // default value
	outline_color(1.,1.,1.) // default value
{

  tex = new Texture3D(w, h, d, NVIS_FLOAT, NVIS_LINEAR_INTERP, n);
  tex->initialize(data);
  id = tex->id;

  aspect_ratio_width = s*tex->aspect_ratio_width;
  aspect_ratio_height = s*tex->aspect_ratio_height;
  aspect_ratio_depth = s*tex->aspect_ratio_depth;

  location = Vector3(x,y,z);

  //Add cut planes. We create 3 default cut planes facing x, y, z directions.
  //The default location of cut plane is in the middle of the data.
  plane.clear();
  plane.push_back(CutPlane(1, 0.5));
  plane.push_back(CutPlane(2, 0.5));
  plane.push_back(CutPlane(3, 0.5));
  
  //initialize the labels  
  strcpy(label[0], "X Label");
  strcpy(label[1], "Y Label");
  strcpy(label[2], "Z Label");
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

void Volume::set_n_slice(int n) { n_slice = n; }
int Volume::get_n_slice() { return n_slice; }

void Volume::set_size(float s) { 
  size = s; 
  aspect_ratio_width = s*tex->aspect_ratio_width;
  aspect_ratio_height = s*tex->aspect_ratio_height;
  aspect_ratio_depth = s*tex->aspect_ratio_depth;
}

float Volume::get_specular() { return specular; }
float Volume::get_diffuse() { return diffuse; }
float Volume::get_opacity_scale() { return opacity_scale; }
void Volume::set_specular(float s) { specular = s; }
void Volume::set_diffuse(float d) { diffuse = d; }
void Volume::set_opacity_scale(float s) { opacity_scale = s; }

void Volume::enable_data() { data_enabled = true; }
void Volume::disable_data() { data_enabled = false; }
bool Volume::data_is_enabled() { return data_enabled; }

void Volume::enable_outline() { outline_enabled = true; }
void Volume::disable_outline() { outline_enabled = false; }
bool Volume::outline_is_enabled() { return outline_enabled; }
void Volume::set_outline_color(float *rgb) {
    outline_color = Color(rgb[0],rgb[1],rgb[2]);
}
void Volume::get_outline_color(float *rgb) {
    outline_color.GetRGB(rgb);
}

void Volume::set_label(int axis, char* txt){
  strcpy(label[axis], txt);
}	
