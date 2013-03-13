/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */

#include <math.h>

#include <vrmath/Rotation.h>
#include <vrmath/Vector3f.h>
#include <vrmath/Quaternion.h>

using namespace vrmath;

void Rotation::set(const Vector3f &vec1, const Vector3f &vec2)
{
    if (vec1 == vec2) {
        x = 0.0f;
        y = 0.0f;
        z = 1.0f;
        angle = 0.0f;
        return;
    }

    Vector3f perp = cross(vec1, vec2);

    float ang = atan2(perp.length(), vec1.dot(vec2));

    perp = perp.normalize();

    x = perp.x;
    y = perp.y;
    z = perp.z;
    angle = ang;
}

void Rotation::set(const Quaternion& quat)
{
    //if (quat.w > 1) quat.normalize();

    angle = acos(quat.w) * 2.0f;
    float scale = sqrt(1 - quat.w * quat.w);
    if (angle < 1.0e-6f) {
        x = 1;
        y = 0;
        z = 0;
    } else if (scale < 1.0e-6f) {
        x = quat.x;
        y = quat.y;
        z = quat.z;
    } else {
        x = quat.x / scale;
        y = quat.y / scale;
        z = quat.z / scale;
    }
}

Quaternion Rotation::getQuaternion() const
{
    Quaternion result;

    result.w = cos(angle / 2.0);
    double sinHalfAngle = sin(angle / 2.0);
    result.x = x * sinHalfAngle;
    result.y = y * sinHalfAngle;
    result.z = z * sinHalfAngle;

    return result;
}
