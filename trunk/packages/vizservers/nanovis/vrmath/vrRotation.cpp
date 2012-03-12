/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <math.h>

#include <vrmath/vrRotation.h>
#include <vrmath/vrVector3f.h>
#include <vrmath/vrQuaternion.h>

void vrRotation::set(const vrVector3f &vec1, const vrVector3f &vec2)
{
    if (vec1 == vec2) {
        x = 0.0f;
        y = 0.0f;
        z = 1.0f;
        angle = 0.0f;
        return;
    }

    vrVector3f cross;
    cross.cross(vec1, vec2);

    float ang = atan2(cross.length(), vec1.dot(vec2));

    cross.normalize();

    x = cross.x;
    y = cross.y;
    z = cross.z;
    angle = ang;
}

void vrRotation::set(const vrQuaternion& quat)
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

vrQuaternion vrRotation::getQuaternion() const
{
    vrQuaternion result;

    result.w = cos(angle / 2.0);
    double sinHalfAngle = sin(angle / 2.0);
    result.x = x * sinHalfAngle;
    result.y = y * sinHalfAngle;
    result.z = z * sinHalfAngle;

    return result;
}
