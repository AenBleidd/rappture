/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Plane.cpp: plane class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <assert.h>

#include "Plane.h"

Plane::Plane(float _a, float _b, float _c, float _d) :
    a(_a),
    b(_b),
    c(_c),
    d(_d)
{
    assert(a != 0 || b != 0 || c != 0);
}

Plane::Plane(float coeffs[4])
{
    a = coeffs[0];
    b = coeffs[1];
    c = coeffs[2];
    d = coeffs[3];

    assert(a != 0 || b != 0 || c != 0);
}

void
Plane::transform(const Mat4x4& mat)
{
    Vector4 coeffs(a, b, c, d);

    Mat4x4 inv = mat.inverse();
    Vector4 new_coeffs = inv.multiplyRowVector(coeffs);
    a = new_coeffs.x;
    b = new_coeffs.y;
    c = new_coeffs.z;
    d = new_coeffs.w;
}

void
Plane::getPoint(Vector3& point)
{
    if (a != 0) {
        point.x = -d/a;
        point.y = 0;
        point.z = 0;
    } else if (b != 0) {
        point.y = -d/b;
        point.x = 0;
        point.z = 0;
    } else if (c != 0) {
        point.z = -d /c;
        point.y = 0;
        point.x = 0;
    } else {
        assert(false);
    }
}
