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
#include "NvVolQDVolume.h"


NvVolQDVolume::NvVolQDVolume(float x, float y, float z,
					int w, int h, int d, float s, int n, 
					float* dataVolumeA, 
					double v0, double v1, const Vector3& cellSize)
		: Volume(x, y, z, w, h, d, s, n, dataVolumeA, v0, v1), cell_size(cellSize)
{
  //label it as zincblende
  volume_type = VOLQD;

  //compute cellsize
  cell_size = Vector3(0.25/w, 0.25/h, 0.25/d);
}


NvVolQDVolume::~NvVolQDVolume()
{ 
}


