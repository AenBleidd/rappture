/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ColorGradient.h: interface for the ColorGradient class 
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

#ifndef COLOR_GRADIENT_H
#define COLOR_GRADIENT_H

#include "../Color.h"
#include "ControlPoint.h"

class ColorGradient
{
public:

	int numOfKeys;	//number of points		
	ControlPoint* keyList;	//single linked list of control points
	Color *keyColors;		//list of key colors

	void SaveColorGradientFile(char *fileName);
	void ReadColorGradientFile(char *fileName);
	static void changeColor(double r, double g, double b);
	ControlPoint* addKey(double key, Color *color);
	void removeKey(double key);
	void cleanUp();
	void scaleKeys(double scaleX, int newUnitWidth);
	ColorGradient();
	virtual ~ColorGradient();
	friend class ColorGradientGLUTWindow;
};

#endif
