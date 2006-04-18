/*
 * ----------------------------------------------------------------------
 * Camera.h : Camera class
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


#include <GL/glu.h>
#include "Vector3.h"

class Camera{

public:
	Vector3 location;	//Location of the camera in the scene
	Vector3 target;		//Location the camera is looking at.
       				//location and target: two points define the line-of-sight	
	Vector3 angle;		//rotation angles of camera along x, y, z
	int width;	//screen size
	int height;	//screen size

	~Camera();
	Camera(int w, int h,
		double loc_x, double loc_y, double loc_z, 
		double target_x, double target_y, double target_z,
		int angle_x, int angle_y, int angle_z);
	void move(double loc_x, double loc_y, double loc_z);	//move location of camera
	void aim(double target_x, double target_y, double target_z); //change target point
	void rotate(double angle_x, double angle_y, double angle_z); //change target point
	void activate();//make the camera setting active, this has to be called before drawing things.
};

#endif
