/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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

    float ang = atan2( cross.length(), vec1.dot(vec2) );

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
    if (scale < 0.001) {
        x = quat.x;
        y = quat.y;
        z = quat.z;
    } else {
        x = quat.x / scale;
        y = quat.y / scale;
        z = quat.z / scale;
    }
    /*
    // http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation#Quaternion_to_axis-angle
    float scale = sqrt(quat.x * quat.x + quat.y * quat.y + quat.z * quat.z);
    x = quat.x / scale;
    y = quat.y / scale;
    z = quat.z / scale;
    */
}
