/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * Quaternion code by BLACKAXE / kolor aka Laurent Schmalen
 * Use for commercial is strictly prohibited
 *
 * I (Insoo Woo) have changed names according to my naming rules
 */
#ifndef VRQUATERNION_H
#define VRQUATERNION_H

#include <vrmath/vrLinmath.h>

class vrRotation;

class vrQuaternion
{
public:
    vrQuaternion();
    explicit vrQuaternion(const vrRotation& rot);
    vrQuaternion(float x, float y = 0, float z = 0, float w = 0);

    void set(float x1, float y1, float z1, float w1);
    const vrQuaternion& set(const vrRotation& rot);

    void slerp(const vrRotation &a,const vrRotation &b, const float t);

    void slerp(const vrQuaternion &a,const vrQuaternion &b, const float t);

    const vrQuaternion& normalize();

    friend bool operator==(const vrQuaternion& q1, const vrQuaternion& q2);

    float x, y, z, w;
};

inline bool operator==(const vrQuaternion& q1, const vrQuaternion& q2)
{
    return ((q1.x == q2.x) && (q1.y == q2.y) && (q1.z == q2.z) && (q1.w == q2.w));
}

inline void vrQuaternion::slerp(const vrRotation &a,const vrRotation &b, const float t)
{
    slerp(vrQuaternion(a), vrQuaternion(b), t);
}

inline void vrQuaternion::set(float x1, float y1, float z1, float w1)
{
    x = x1;
    y = y1;
    z = z1;
    w = w1;
}

#endif
