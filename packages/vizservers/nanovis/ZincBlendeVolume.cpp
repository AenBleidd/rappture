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
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <assert.h>
#include "ZincBlendeVolume.h"

ZincBlendeVolume::ZincBlendeVolume(float x, float y, float z,
                                   int w, int h, int d, float s, int n, 
                                   float* dataVolumeA, float* dataVolumeB,
                                   double v0, double v1, double non_zeromin,
                                   const Vector3& cellSize) :
    Volume(x, y, z, w, h, d, s, n, dataVolumeA, v0, v1, non_zeromin),
    cell_size(cellSize)
{
    //label it as zincblende
    _volume_type = ZINCBLENDE;

    //store member tex initialize in Volume() as zincblende_tex[0]
    assert(_tex);
    zincblende_tex[0] = _tex;

    //now add another tex as zincblende_tex[1]
    Texture3D* secondTex = new Texture3D(w, h, d, GL_FLOAT, GL_LINEAR, n);
    assert(secondTex);
    secondTex->initialize(dataVolumeB);

    zincblende_tex[1] = secondTex;
}

ZincBlendeVolume::~ZincBlendeVolume()
{ 
    // This data will be deleted in a destrutor of Volume class
    //if(zincblende_tex[0])
    //  delete zincblende_tex[0];
    if(zincblende_tex[1])
	delete zincblende_tex[1];
}
