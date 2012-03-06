/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * NvCamera.h : NvCamera class
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

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "Vector3.h"

class NvCamera {

    Vector3 location_;		//Location of the camera in the scene
    Vector3 target_;		//Location the camera is looking at.  
				//location and target: two points define the
                                //line-of-sight
    Vector3 angle_;		//rotation angles of camera along x, y, z
    int width_;			//screen width
    int height_;		//screen height
    int startX_;
    int startY_;

public:
    ~NvCamera(void) {
	/*empty*/
    }	 
    NvCamera(int startx, int starty, int w, int h,
             float loc_x, float loc_y, float loc_z, 
             float target_x, float target_y, float target_z,
             float angle_x, float angle_y, float angle_z);

    //move location of camera
    void x(float loc_x) {
	location_.x = loc_x;
    }
    float x(void) {
	return location_.x;
    }
    void y(float loc_y) {
	location_.y = loc_y;
    }
    float y(void) {
	return location_.y;
    }
    void z(float loc_z) {
	location_.z = loc_z;
    }
    float z(void) {
	return location_.z;
    }

    //move location of target
    void xAim(float x) {
	target_.x = x;
    }
    float xAim(void) {
	return target_.x;
    }
    void yAim(float y) {
	target_.y = y;
    }
    float yAim(void) {
	return target_.y;
    }
    void zAim(float z) {
	target_.z = z;
    }
    float zAim(void) {
	return target_.z;
    }

    void rotate(float angle_x, float angle_y, float angle_z) { 
	angle_ = Vector3(angle_x, angle_y, angle_z);
    }
    void rotate(Vector3 angle) { 
	angle_ = angle;
    }
    Vector3 rotate(void) { 
	return angle_;
    }
    void set_screen_size(int sx, int sy, int w, int h) {
	width_ = w, height_ = h;
	startX_ = sx, startY_ = sy;
    }
    void initialize(void); //make the camera setting active, this has to be
			 //called before drawing things.
};

#endif
