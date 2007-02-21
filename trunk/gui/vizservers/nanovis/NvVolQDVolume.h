/*
 * ----------------------------------------------------------------------
 * ZincBlendeVolume.h: 3d zincblende volume class, a subclass of Volume. 
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

#ifndef _NV_VOLQD_VOLUME_H_
#define _NV_VOLQD_VOLUME_H_


#include "Volume.h"


using namespace std;


class NvVolQDVolume : public Volume
{
public:
	Vector3 cell_size;	//the cell size in texture space


	NvVolQDVolume(float x, float y, float z, 
		int width, int height, int depth, float size, int n_component, 
		float* dataVolumeA, 
		double vmin, double vmax, const Vector3& cellSize);

	~NvVolQDVolume();
};

#endif
