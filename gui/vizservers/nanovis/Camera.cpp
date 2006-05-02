/*
 * ----------------------------------------------------------------------
 * Camera.cpp : Camera class
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


#include "Camera.h"

Camera::Camera(int w, int h,
		double loc_x, double loc_y, double loc_z, 
		double target_x, double target_y, double target_z,
		int angle_x, int angle_y, int angle_z):
	 width(w),
	 height(h),
	 location(Vector3(loc_x, loc_y, loc_z)),
	 target(Vector3(target_x, target_y, target_z)),
	 angle(Vector3(angle_x, angle_y, angle_z)) { }

Camera::~Camera(){}	 

void Camera::move(double loc_x, double loc_y, double loc_z)
{ 
  location = Vector3(loc_x, loc_y, loc_z); 
}

void Camera::aim(double target_x, double target_y, double target_z)
{ 
  target = Vector3(target_x, target_y, target_z);
}

void Camera::rotate(double angle_x, double angle_y, double angle_z)
{ 
  angle = Vector3(angle_x, angle_y, angle_z);
}

void Camera::activate(){
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, (GLdouble)1, 0.1, 50.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  gluLookAt(location.x, location.y, location.z,
            target.x, target.y, target.z,
            0., 1., 0.);

   glRotated(angle.x, 1., 0., 0.);
   glRotated(angle.y, 0., 1., 0.);
   glRotated(angle.z, 0., 0., 1.);
}

void Camera::set_screen_size(int w, int h){
  width = w; height = h;
}
