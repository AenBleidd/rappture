/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Authors:
 *   Wei Qiao <qiaow@purdue.edu>
 */
#ifndef NV_ZINCBLENDE_VOLUME_H
#define NV_ZINCBLENDE_VOLUME_H

#include <vrmath/Vector3f.h>

#include "Volume.h"

namespace nv {

/**
 * \brief 3D ZincBlende volume, contains two cubic volumes
 */
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
