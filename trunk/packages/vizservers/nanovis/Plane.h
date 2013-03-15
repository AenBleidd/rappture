/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Plane.h: plane class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef PLANE_H
#define PLANE_H

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>
#include <vrmath/Matrix4x4d.h>

namespace nv {

class Plane
{
public:
    Plane(float a, float b, float c, float d);

    Plane(float coeffs[4]);

    Plane()
    {}

    void getPoint(vrmath::Vector3f& point);

    //bool clips(float point[3]) const { return !retains(point); }

    void transform(const vrmath::Matrix4x4d& mat);

    void transform(float *m)
    {
        vrmath::Matrix4x4d mat;
        mat.setFloat(m);
        transform(mat);
    }

    bool retains(const vrmath::Vector3f& point) const
    {
	return ((a*point.x + b*point.y + c*point.z + d) >= 0);
    }

    vrmath::Vector4f getCoeffs() const
    {
        return vrmath::Vector4f(a, b, c, d);
    }

    void setCoeffs(float a_val, float b_val, float c_val, float d_val)
    {
        a = a_val, b = b_val, c = c_val, d = d_val;
    }

    void getNormal(vrmath::Vector3f& normal) const
    {
        normal.x = a;
        normal.y = b;
        normal.z = c;
    }

private:
    float a, b, c, d;
};

}

#endif
