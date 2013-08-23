/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Volume.cpp: 3d volume class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <memory.h>
#include <assert.h>
#include <float.h>

#include <vrmath/Vector4f.h>
#include <vrmath/Matrix4x4d.h>

#include "Volume.h"
#include "Trace.h"

using namespace nv;
using namespace vrmath;

bool Volume::updatePending = false;
double Volume::valueMin = 0.0;
double Volume::valueMax = 1.0;

Volume::Volume(int w, int h, int d,
               int n, float *data,
               double v0, double v1, double nonZeroMin) :
    _id(0),
    _width(w),
    _height(h),
    _depth(d),
    _transferFunc(NULL),
    _ambient(0.6f),
    _diffuse(0.4f),
    _specular(0.3f),
    _specularExp(90.0f),
    _lightTwoSide(false),
    _opacityScale(0.5f),
    _data(NULL),
    _numComponents(n),
    _nonZeroMin(nonZeroMin),
    _cutplanesVisible(true),
    _tex(NULL),
    _position(0,0,0),
    _scale(1,1,1),
    _numSlices(512),
    _enabled(true),
    _dataEnabled(true),
    _outlineEnabled(true),
    _volumeType(CUBIC),
    _isosurface(0)
{
    TRACE("Enter: %dx%dx%d", _width, _height, _depth);

    _outlineColor[0] = _outlineColor[1] = _outlineColor[2] = 1.0f;

    _tex = new Texture3D(_width, _height, _depth, GL_FLOAT, GL_LINEAR, n);
    int fcount = _width * _height * _depth * _numComponents;
    _data = new float[fcount];
    memcpy(_data, data, fcount * sizeof(float));
    _tex->initialize(_data);

    _id = _tex->id();

    wAxis.setRange(v0, v1);

    //Add cut planes. We create 3 default cut planes facing x, y, z directions.
    //The default location of cut plane is in the middle of the data.
    _plane.clear();
    addCutplane(CutPlane::X_AXIS, 0.5f);
    addCutplane(CutPlane::Y_AXIS, 0.5f);
    addCutplane(CutPlane::Z_AXIS, 0.5f);

    TRACE("Leave");
}

Volume::~Volume()
{
    TRACE("Enter");

    delete [] _data;
    delete _tex;
}

void Volume::setData(float *data, double v0, double v1, double nonZeroMin)
{
    int fcount = _width * _height * _depth * _numComponents;
    memcpy(_data, data, fcount * sizeof(float));
    _tex->update(_data);
    wAxis.setRange(v0, v1);
    _nonZeroMin = nonZeroMin;
    updatePending = true;
}

void Volume::getBounds(Vector3f& bboxMin, Vector3f& bboxMax) const
{
    bboxMin.set(xAxis.min(), yAxis.min(), zAxis.min());
    bboxMax.set(xAxis.max(), yAxis.max(), zAxis.max());
}
