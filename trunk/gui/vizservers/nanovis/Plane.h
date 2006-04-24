/*
 * ----------------------------------------------------------------------
 * Plane.h: plane class
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
#ifndef _PLANE_H_
#define _PLANE_H_

#include "Vector3.h"
#include "Mat4x4.h"

class Plane {
public:
	float a, b, c, d;
	
    Plane(float a, float b, float c, float d);
    Plane(float coeffs[4]);
    Plane();
    
    void get_normal(Vector3 &normal);
    void get_point(Vector3 &point);
    Vector4 get_coeffs();
    void set_coeffs(float _a, float _b, float _c, float _d);
    void transform(Mat4x4 mat);
    void transform(float *m);
    bool retains(Vector3 point);
    //bool clips(float point[3]) const { return !retains(point); }

};


#endif
