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

#include <memory.h>
#include <assert.h>
#include "Volume.h"
#include "Trace.h"

bool Volume::update_pending = false;
double Volume::valueMin = 0.0;
double Volume::valueMax = 1.0;

Volume::Volume(
		float x, float y, float z,
		int w, int h, int d, float s, 
		int n, float* data, double v0, double v1, double nz_min):
    width(w),
    height(h),
    depth(d),
    size(s),
    n_components(n),
    nonzero_min(nz_min),
    pointsetIndex(-1),
    enabled(true),
    //n_slice(256),		// default value
    n_slice(512),		// default value
    _specular(6.),		// default value
    _diffuse(3.),		// default value
    _opacity_scale(10.),	// default value
    data_enabled(true),		// default value
    outline_enabled(true),	// default value
    outline_color(1.,1.,1.),	// default value
    volume_type(CUBIC),		//by default, is a cubic volume
    iso_surface(0)
{
    tex = new Texture3D(w, h, d, NVIS_FLOAT, NVIS_LINEAR_INTERP, n);
    int fcount = width * height * depth * n_components;
    _data = new float[fcount];
    if (data != NULL) {
        Trace("data is copied\n");
        memcpy(_data, data, fcount * sizeof(float));
        tex->initialize(_data);
    } else {
        Trace("data is null\n");
        memset(_data, 0, sizeof(width * height * depth * n_components * 
				sizeof(float)));
        tex->initialize(_data);
    }

    id = tex->id;
    
    wAxis.SetRange(v0, v1);

	// VOLUME
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
    label[0] = "X Label";
    label[1] = "Y Label";
    label[2] = "Z Label";

    Trace("End -- Volume constructor\n");
}

Volume::~Volume()
{ 
    if (pointsetIndex != -1)
    {
        // TBD...
    }

    delete [] _data;

    delete tex; 
}
