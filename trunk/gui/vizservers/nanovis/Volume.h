/*
 * ----------------------------------------------------------------------
 * Volume.h: 3d volume class
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

#ifndef _VOLUME_H_
#define _VOLUME_H_

#include <vector>

#include "define.h"
#include "Texture3D.h"
#include "Vector3.h"

using namespace std;

struct CutPlane{
  int orient;	//orientation - 1: xy slice, 2: yz slice, 3: xz slice
  float offset;	//normalized offset [0,1] in the volume
  bool enabled;

  CutPlane(int _orient, float _offset):
	orient(_orient),
	offset(_offset),
	enabled(true){}
};


class Volume{
	
private:
	vector <CutPlane> plane; //cut planes

public:
	Vector3 location;

	bool enabled; 

	int width;
	int height;
	int depth;

	float aspect_ratio_width;
	float aspect_ratio_height;
	float aspect_ratio_depth;

	NVISdatatype type;
	NVISinterptype interp_type;
	int n_components;

	Texture3D* tex;	//OpenGL texture storing the volume
	NVISid id;   //OpenGL textue identifier (==tex->id)

	Volume(float x, float y, float z, 
			int width, int height, int depth, 
			NVISdatatype type, NVISinterptype interp,
			int n_component, float* data);
	~Volume();
	
	void enable(); //VolumeRenderer will render an enabled volume and its cutplanes
	void disable();
	void move(Vector3 _loc);
	bool is_enabled();
	Vector3* get_location();

	//methods related to cutplanes
        int add_cutplane(int _orientation, float _location); //add a plane and returns its index
        void enable_cutplane(int index);
        void disable_cutplane(int index);
        void move_cutplane(int index, float _location);
	CutPlane* get_cutplane(int index);
	int get_cutplane_count();	//returns the number of cutplanes in the volume
	bool cutplane_is_enabled(int index);  	//check if a cutplane is enabled

};

#endif
