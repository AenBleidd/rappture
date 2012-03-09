/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * NvCamera.cpp : NvCamera class
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
#include <stdio.h>

#include <GL/glew.h>
#include <GL/glu.h>

#include "NvCamera.h"
#include "Trace.h"

NvCamera::NvCamera(int startx, int starty, int w, int h,
		   float loc_x, float loc_y, float loc_z, 
		   float target_x, float target_y, float target_z,
		   float angle_x, float angle_y, float angle_z) :
    _location(loc_x, loc_y, loc_z),
    _target(target_x, target_y, target_z),
    _angle(angle_x, angle_y, angle_z),
    _width(w),
    _height(h),
    _startX(startx),
    _startY(starty)
{
}

void 
NvCamera::initialize()
{
    TRACE("camera: %d, %d\n", _width, _height);
    glViewport(_startX, _startY, _width, _height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(30, 
		   (GLdouble)(_width - _startX)/(GLdouble)(_height - _startY), 
		   0.1, 50.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(_location.x, _location.y, _location.z,
	      _target.x, _target.y, _target.z,
	      0., 1., 0.);

    glRotated(_angle.x, 1., 0., 0.);
    glRotated(_angle.y, 0., 1., 0.);
    glRotated(_angle.z, 0., 0., 1.);
}

