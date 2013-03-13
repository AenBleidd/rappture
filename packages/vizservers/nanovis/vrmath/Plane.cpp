/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */

#include <vrmath/Plane.h>
#include <vrmath/Matrix4x4f.h>

using namespace vrmath;

void Plane::transform(Matrix4x4f& mat)
{
    Vector4f v(normal.x, normal.y, normal.z, distance);
    float* m = mat.get();

    normal.set(m[0]*v.x + m[1]*v.y + m[2]*v.z + m[3]*v.w,
               m[4]*v.x + m[5]*v.y + m[6]*v.z + m[7]*v.w,
               m[8]*v.x + m[9]*v.y + m[10]*v.z + m[11]*v.w);

    distance = m[12]*v.x + m[13]*v.y + m[14]*v.z + m[15]*v.w;
}
