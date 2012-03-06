/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ScreenSnapper.h: ScreenSnapper class. It captures the render result
 * 			and stores it in an array of chars or floats 
 * 			depending on chosen format.
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

#ifndef _SCREEN_SNAPPER_H_ 
#define _SCREEN_SNAPPER_H_

#include "define.h"
#include <GL/gl.h>
#include <stdio.h>


class ScreenSnapper 
{
public:
	int width, height;	//size of the screen
	GLuint data_type;	//data type: GL_FLOAT or GL_UNSIGNED_BYTE
	int n_channels_per_pixel; //RGB(3) or RGBA(4)
	
	void* data;  //storage array for the captured image. This array is "flat".
		//It stores pixels in the order from lower-left corner to upper-right corner.
		//[rgb][rgb][rgb]... or [rgba][rgba][rgba]...

	ScreenSnapper(int width, int height, GLuint type, int channel_per_pixel);
	~ScreenSnapper();

	void reset(char c);	//set every byte in the data array to c
	void capture();
	void print();
};

#endif
