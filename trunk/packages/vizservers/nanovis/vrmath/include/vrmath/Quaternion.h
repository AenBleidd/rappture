/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */
#ifndef VRQUATERNION_H
#define VRQUATERNION_H

namespace vrmath {

class Rotation;

class Quaternion
{
public:
    Quaternion();

    explicit Quaternion(const Rotation& rot);

    Quaternion(double x, double y, double z, double w);

    void set(double x1, double y1, double z1, double w1);

    const Quaternion& set(const Rotation& rot);

    Quaternion conjugate() const
    {
        Quaternion result;
        result.w = w;
        result.x = -x;
        result.y = -y;
        result.z = -z;
        return result;
    }

    Quaternion reciprocal() const
    {
        double denom =  w*w + x*x + y*y + z*z;
        Quaternion result = conjugate();
        result.x /= denom;
        result.y /= denom;
        result.z /= denom;
        result.w /= denom;
        return result;
    }

    void slerp(const Rotation &a,const Rotation &b, double t);

    void slerp(const Quaternion &a,const Quaternion &b, double t);

    Quaternion& normalize();

    Quaternion operator*(const Quaternion& q2) const
    {
        Quaternion result;
        result.w = (w * q2.w) - (x * q2.x) - (y * q2.y) - (z * q2.z);
        result.x = (w * q2.x) + (x * q2.w) + (y * q2.z) - (z * q2.y);
        result.y = (w * q2.y) + (y * q2.w) + (z * q2.x) - (x * q2.z);
        result.z = (w * q2.z) + (z * q2.w) + (x * q2.y) - (y * q2.x);
        return result;
    }

    friend bool operator==(const Quaternion& q1, const Quaternion& q2);

    double x, y, z, w;
};

inline bool operator==(const Quaternion& q1, const Quaternion& q2)
{
    return ((q1.x == q2.x) && (q1.y == q2.y) && (q1.z == q2.z) && (q1.w == q2.w));
}

inline void Quaternion::slerp(const Rotation &a,const Rotation &b, double t)
{
    slerp(Quaternion(a), Quaternion(b), t);
}

inline void Quaternion::set(double x1, double y1, double z1, double w1)
{
    x = x1;
    y = y1;
    z = z1;
    w = w1;
}

}

#endif
