/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
	double R;		// Red component
	double G;		// Green component
	double B;		// Blue component

	void GetRGB(unsigned char *result);
	void GetRGB(float *result);

	void SetRGBA(unsigned char *color);
	void GetRGBA(double opacity, unsigned char *result);
	Color operator *(Color &other);
	Color* next;	//pointer to the next color

	Color();
	Color(double r, double g, double b);
	Color(const Color& c);
	Color& operator=(const Color& c);
	~Color();

	void LimitColors(); //Limits the color to be in range of 0.0 and 1.0
	Color operator*(double k);
	friend Color operator*(double k, Color &other);
	Color operator+(Color &other);
};

#endif
