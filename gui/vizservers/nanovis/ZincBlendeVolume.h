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

#ifndef _ZINCBLENDE_VOLUME_H_
#define _ZINCBLENDE_VOLUME_H_


#include "Volume.h"


using namespace std;


class ZincBlendeVolume : public Volume
{
	
public:
	Texture3D* zincblende_tex[2];	//the textures of two cubic volumes
	Vector3 cell_size;	//the cell size in texture space


	ZincBlendeVolume(float x, float y, float z, 
		int width, int height, int depth, float size, int n_component, 
		float* dataVolumeA, float* dataVolumeB,
		double vmin, double vmax);

	~ZincBlendeVolume();
};

#endif
