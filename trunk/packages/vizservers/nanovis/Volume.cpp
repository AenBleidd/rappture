/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Volume.cpp: 3d volume class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
               int w, int h, int d,
               int n, float *data,
               double v0, double v1, double nonZeroMin) :
    _id(0),
    _width(w),
    _height(h),
    _depth(d),
    _tfPtr(NULL),
    _ambient(0.6f),
    _diffuse(0.4f),
    _specular(0.3f),
    _specularExp(90.0f),
    _lightTwoSide(false),
    _opacityScale(0.5f),
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
    TRACE("Enter: %dx%dx%d", _width, _height, _depth);

    _tex = new Texture3D(_width, _height, _depth, GL_FLOAT, GL_LINEAR, n);
    int fcount = _width * _height * _depth * _numComponents;
    _data = new float[fcount];
    if (data != NULL) {
        TRACE("data is copied");
        memcpy(_data, data, fcount * sizeof(float));
        _tex->initialize(_data);
    } else {
        TRACE("data is null");
        memset(_data, 0, sizeof(_width * _height * _depth * _numComponents * 
				sizeof(float)));
        _tex->initialize(_data);
    }

    _id = _tex->id();

    wAxis.setRange(v0, v1);

    //Add cut planes. We create 3 default cut planes facing x, y, z directions.
    //The default location of cut plane is in the middle of the data.
    _plane.clear();
    addCutplane(1, 0.5f);
    addCutplane(2, 0.5f);
    addCutplane(3, 0.5f);

    TRACE("Leave");
}

Volume::~Volume()
{
    TRACE("Enter");

    delete [] _data;
    delete _tex;
}
