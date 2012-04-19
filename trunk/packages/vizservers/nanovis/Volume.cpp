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

bool Volume::updatePending = false;
double Volume::valueMin = 0.0;
double Volume::valueMax = 1.0;

Volume::Volume(float x, float y, float z,
               int w, int h, int d, float s, 
               int n, float *data,
               double v0, double v1, double nonZeroMin) :
    aspectRatioWidth(1),
    aspectRatioHeight(1),
    aspectRatioDepth(1),
    id(0),
    width(w),
    height(h),
    depth(d),
    size(s),
    pointsetIndex(-1),
    _tfPtr(NULL),
    _specular(6.),
    _diffuse(3.),
    _opacityScale(10.),
    _name(NULL),
    _data(NULL),
    _numComponents(n),
    _nonZeroMin(nonZeroMin),
    _tex(NULL),
    _location(x, y, z),
    _numSlices(512),
    _enabled(true),
    _dataEnabled(true),
    _outlineEnabled(true),
    _outlineColor(1., 1., 1.),
    _volumeType(CUBIC),
    _isosurface(0)
{
    _tex = new Texture3D(w, h, d, GL_FLOAT, GL_LINEAR, n);
    int fcount = width * height * depth * _numComponents;
    _data = new float[fcount];
    if (data != NULL) {
        TRACE("data is copied\n");
        memcpy(_data, data, fcount * sizeof(float));
        _tex->initialize(_data);
    } else {
        TRACE("data is null\n");
        memset(_data, 0, sizeof(width * height * depth * _numComponents * 
				sizeof(float)));
        _tex->initialize(_data);
    }

    id = _tex->id();

    wAxis.setRange(v0, v1);

    // VOLUME
    aspectRatioWidth  = s * _tex->aspectRatioWidth();
    aspectRatioHeight = s * _tex->aspectRatioHeight();
    aspectRatioDepth =  s * _tex->aspectRatioDepth();

    //Add cut planes. We create 3 default cut planes facing x, y, z directions.
    //The default location of cut plane is in the middle of the data.
    _plane.clear();
    addCutplane(1, 0.5f);
    addCutplane(2, 0.5f);
    addCutplane(3, 0.5f);

    TRACE("End -- Volume constructor\n");
}

Volume::~Volume()
{ 
    if (pointsetIndex != -1) {
        // TBD...
    }

    delete [] _data;
    delete _tex;
}
