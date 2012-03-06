/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vrmath/vrPlane.h>

void vrPlane::transform(vrMatrix4x4f& mat)
{
    vrVector4f v(normal.x, normal.y, normal.z, distance);
    float* m = mat.get();

    normal.set(m[0]*v.x + m[1]*v.y + m[2]*v.z + m[3]*v.w,
                    m[4]*v.x + m[5]*v.y + m[6]*v.z + m[7]*v.w,
                    m[8]*v.x + m[9]*v.y + m[10]*v.z + m[11]*v.w);

    distance = m[12]*v.x + m[13]*v.y + m[14]*v.z + m[15]*v.w;
}
