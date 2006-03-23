/*
 * ----------------------------------------------------------------------
 * Color.h: RGBA color class
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
#ifndef _COLOR_H_ 
#define _COLOR_H_

class Color  
{
public:

	float r, g, b, a;

	Color(){};
	Color(float _r, float _g, float _b, float _a);
	~Color(){};
};

#endif
