/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
#ifndef PLANE_H
#define PLANE_H

#include "Vector3.h"
#include "Mat4x4.h"

class Plane
{
    float a, b, c, d;
public:

    Plane(float a, float b, float c, float d);

    Plane(float coeffs[4]);

    Plane()
    {}

    void getPoint(Vector3& point);

    //bool clips(float point[3]) const { return !retains(point); }

    void transform(const Mat4x4& mat);

    void transform(float *m)
    {
        Mat4x4 mat(m);
        transform(mat);
    }

    bool retains(const Vector3& point) const
    {
	return ((a*point.x + b*point.y + c*point.z + d) >= 0);
    }

    Vector4 getCoeffs() const
    {
        return Vector4(a, b, c, d);
    }

    void setCoeffs(float a_val, float b_val, float c_val, float d_val)
    {
        a = a_val, b = b_val, c = c_val, d = d_val;
    }

    void getNormal(Vector3& normal) const
    {
        normal.x = a;
        normal.y = b;
        normal.z = c;
    }
};


#endif
