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
#include "Trace.h"
#include <GL/glu.h>
#include "NvCamera.h"

NvCamera::NvCamera(int startx, int starty, int w, int h,
		   float loc_x, float loc_y, float loc_z, 
		   float target_x, float target_y, float target_z,
		   float angle_x, float angle_y, float angle_z):
    location_(Vector3(loc_x, loc_y, loc_z)),
    target_(Vector3(target_x, target_y, target_z)),
    angle_(Vector3(angle_x, angle_y, angle_z)),
    width_(w),
    height_(h),
    startX_(startx),
    startY_(starty)
{ 
    /*empty*/
}


void 
NvCamera::initialize()
{
    TRACE("camera: %d, %d\n", width_, height_);
    glViewport(startX_, startY_, width_, height_);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(30, 
		   (GLdouble)(width_ - startX_)/(GLdouble)(height_ - startY_), 
		   0.1, 50.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(location_.x, location_.y, location_.z,
	      target_.x, target_.y, target_.z,
	      0., 1., 0.);

    glRotated(angle_.x, 1., 0., 0.);
    glRotated(angle_.y, 0., 1., 0.);
    glRotated(angle_.z, 0., 0., 1.);
}

