/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */

#include <cstdlib>
#include <cmath>
#include <float.h>

#include <vrmath/Quaternion.h>
#include <vrmath/Rotation.h>
#include <vrmath/Vector3f.h>

#ifndef PI
#define PI 3.14159264
#endif

using namespace vrmath;

Quaternion::Quaternion() :
    x(1.0f), y(0.0f), z(0.0f), w(0.0f)
{
}

Quaternion::Quaternion(const Rotation& rot)
{
    set(rot);
}

Quaternion::Quaternion(double x1, double y1, double z1, double w1) :
    x(x1), y(y1), z(z1), w(w1)
{
}

Quaternion& Quaternion::normalize()
{
    double n = sqrt(w*w + x*x + y*y + z*z);
    if (n == 0.0) {
        w = 1.0; 
        x = y = z = 0.0;
    } else {
        double recip = 1.0/n;
        w *= recip;
        x *= recip;
        y *= recip;
        z *= recip;
    }

    return *this;
}

const Quaternion& Quaternion::set(const Rotation& rot)
{
    Vector3f q(rot.x, rot.y, rot.z);
    q.normalize();
    double s = sin(rot.angle * 0.5);
    x = s * q.x;
    y = s * q.y;
    z = s * q.z;
    w = cos(rot.angle * 0.5);

    return *this;
}

void Quaternion::slerp(const Quaternion &a,const Quaternion &b, double t)
{
    double alpha, beta;
    double cosom = a.x * b.x + a.y * b.y + 
        a.z * b.z + a.w * b.w;

    bool flip = (cosom < 0.0);
    if (flip) cosom = -cosom;
    if ((1.0 - cosom) > 0.00001) {
        double omega = acos(cosom);
        double sinom = sin(omega);
        alpha = sin((1.0 - t) * omega) / sinom;
        beta = sin(t * omega) / sinom;
    } else {
        alpha = 1.0 - t;
        beta = t;
    }

    if (flip) beta = -beta;
    x = alpha * a.x + beta * b.x;
    y = alpha * a.y + beta * b.y;
    z = alpha * a.z + beta * b.z;
    w = alpha * a.w + beta * b.w;
}
