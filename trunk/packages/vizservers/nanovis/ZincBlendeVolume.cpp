/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ZincBlendeVolume.cpp: 3d zincblende volume class, a subclass of Volume.
 * 			It contains two cubic volumes.
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
#include <assert.h>

#include <vrmath/Vector3f.h>

#include "ZincBlendeVolume.h"

using namespace nv;
using namespace vrmath;

ZincBlendeVolume::ZincBlendeVolume(int width, int height, int depth,
                                   int numComponents, 
                                   float *dataVolumeA, float *dataVolumeB,
                                   double vmin, double vmax, double nonZeroMin,
                                   const Vector3f& cellSz) :
    Volume(width, height, depth, numComponents, dataVolumeA, vmin, vmax, nonZeroMin),
    cellSize(cellSz)
{
    //label it as zincblende
    _volumeType = ZINCBLENDE;

    //store member tex initialize in Volume() as zincblendeTex[0]
    assert(_tex);
    zincblendeTex[0] = _tex;

    //now add another tex as zincblende_tex[1]
    Texture3D *secondTex = new Texture3D(width, height, depth, GL_FLOAT, GL_LINEAR, numComponents);
    assert(secondTex);
    secondTex->initialize(dataVolumeB);

    zincblendeTex[1] = secondTex;
}

ZincBlendeVolume::~ZincBlendeVolume()
{ 
    // First texture will be deleted in ~Volume()
    if (zincblendeTex[1])
	delete zincblendeTex[1];
}
