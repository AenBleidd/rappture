/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ZincBlendeVolume.h: 3d zincblende volume class, a subclass of Volume. 
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
#ifndef NV_ZINCBLENDE_VOLUME_H
#define NV_ZINCBLENDE_VOLUME_H

#include <vrmath/Vector3f.h>

#include "Volume.h"

namespace nv {

class ZincBlendeVolume : public Volume
{
public:
    ZincBlendeVolume(float x, float y, float z, 
                     int width, int height, int depth, int numComponents, 
                     float *dataVolumeA, float *dataVolumeB,
                     double vmin, double vmax, double non_zeromin, const vrmath::Vector3f& cellSize);

    virtual ~ZincBlendeVolume();

    Texture3D *zincblendeTex[2]; //the textures of two cubic volumes
    vrmath::Vector3f cellSize;	//the cell size in texture space
};

}

#endif
