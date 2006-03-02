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

#include "define.h"
#include "Texture3D.h"

class Volume{
	
public:
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

	Volume(int width, int height, int depth, 
			NVISdatatype type, NVISinterptype interp,
			int n_component, float* data);
	~Volume();
	
	void activate();
	void deactivate();
	void initialize(float* data);

};

#endif
