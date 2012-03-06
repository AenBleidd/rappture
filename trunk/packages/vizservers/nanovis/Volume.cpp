/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
    _tfPtr(NULL),
    _specular(6.),		// default value
    _diffuse(3.),		// default value
    _opacity_scale(10.),	// default value
    _name(NULL),
    _n_components(n),
    _nonzero_min(nz_min),
    _pointsetIndex(-1),
    _n_slices(512),		// default value
    _enabled(true),
    //n_slice(256),		// default value
    _data_enabled(true),		// default value
    _outline_enabled(true),	// default value
    _outline_color(1.,1.,1.),	// default value
    _volume_type(CUBIC),		//by default, is a cubic volume
    _iso_surface(0),
    width(w),
    height(h),
    depth(d),
    size(s)
{
    _tex = new Texture3D(w, h, d, GL_FLOAT, GL_LINEAR, n);
    int fcount = width * height * depth * _n_components;
    _data = new float[fcount];
    if (data != NULL) {
        TRACE("data is copied\n");
        memcpy(_data, data, fcount * sizeof(float));
        _tex->initialize(_data);
    } else {
        TRACE("data is null\n");
        memset(_data, 0, sizeof(width * height * depth * _n_components * 
				sizeof(float)));
        _tex->initialize(_data);
    }

    id = _tex->id;
    
    wAxis.SetRange(v0, v1);

    // VOLUME
    aspect_ratio_width  = s * _tex->aspect_ratio_width;
    aspect_ratio_height = s * _tex->aspect_ratio_height;
    aspect_ratio_depth =  s * _tex->aspect_ratio_depth;
    
    _location = Vector3(x,y,z);
    
    //Add cut planes. We create 3 default cut planes facing x, y, z directions.
    //The default location of cut plane is in the middle of the data.
    _plane.clear();
    _plane.push_back(CutPlane(1, 0.5));
    _plane.push_back(CutPlane(2, 0.5));
    _plane.push_back(CutPlane(3, 0.5));
    
    //initialize the labels  
    label[0] = "X Label";
    label[1] = "Y Label";
    label[2] = "Z Label";

    TRACE("End -- Volume constructor\n");
}

Volume::~Volume()
{ 
    if (_pointsetIndex != -1)
    {
        // TBD...
    }

    delete [] _data;

    delete _tex; 
}
