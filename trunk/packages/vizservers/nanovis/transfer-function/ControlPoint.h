/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ControlPoint.h
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

#ifndef _CONTROL_POINT_H_
#define _CONTROL_POINT_H_

class ControlPoint  
{
public:
	
	double x;
	double y;

	bool selected;
	bool dragged;

	ControlPoint * next;

public:
	void Set(double x, double y);
	void glDraw();		//draw a cross
	void glDraw_2();	//draw a filled squre
	void glDraw_3();	//draw a circle
	ControlPoint(double _x, double _y);
	virtual ~ControlPoint();

};

#endif
