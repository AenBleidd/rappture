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
#include "NvCamera.h"

NvCamera::NvCamera(int startx, int starty, int w, int h,
		double loc_x, double loc_y, double loc_z, 
		double target_x, double target_y, double target_z,
		int angle_x, int angle_y, int angle_z):
     startX(startx),
     startY(starty),
	 width(w),
	 height(h),
	 location(Vector3(loc_x, loc_y, loc_z)),
	 target(Vector3(target_x, target_y, target_z)),
	 angle(Vector3(angle_x, angle_y, angle_z)) { }

NvCamera::~NvCamera(){}	 

void NvCamera::move(double loc_x, double loc_y, double loc_z)
{ 
  location = Vector3(loc_x, loc_y, loc_z); 
}

void NvCamera::aim(double target_x, double target_y, double target_z)
{ 
  target = Vector3(target_x, target_y, target_z);
}

void NvCamera::rotate(double angle_x, double angle_y, double angle_z)
{ 
  angle = Vector3(angle_x, angle_y, angle_z);
}

void NvCamera::activate(){
  //fprintf(stderr, "camera: %d, %d\n", width, height);
  glViewport(startX, startY, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(30, (GLdouble)(width - startX)/(GLdouble)(height - startY), 0.1, 50.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  gluLookAt(location.x, location.y, location.z,
            target.x, target.y, target.z,
            0., 1., 0.);

   glRotated(angle.x, 1., 0., 0.);
   glRotated(angle.y, 0., 1., 0.);
   glRotated(angle.z, 0., 0., 1.);
}

void NvCamera::set_screen_size(int sx, int sy, int w, int h){
  width = w; height = h;
  startX = sx; startY = sy;
}
