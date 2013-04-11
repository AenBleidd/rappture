/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */
#ifndef VRROTATION_H
#define VRROTATION_H

#include <vrmath/Vector3f.h>

namespace vrmath {

class Quaternion;

/**
 * Represents an axis/angle rotation (angle in radians)
 */
class Rotation
{
public:
    Rotation() :
        x(0.0), y(1.0), z(0.0), angle(0.0)
    {}

    Rotation(double x1, double y1, double z1, double angle1) :
        x(x1), y(y1), z(z1), angle(angle1)
    {}

    Rotation(const Rotation& other) :
        x(other.x), y(other.y), z(other.z), angle(other.angle)
    {}

    Rotation& operator=(const Rotation& other)
    {
        if (&other != this) {
            set(other.x, other.y, other.z, other.angle);
        }
        return *this;
    }

    double getX() const
    { return x; }
    double getY() const
    { return y; }
    double getZ() const
    { return z; }

    double getAngle() const
    { return angle; }

    Quaternion getQuaternion() const;

    void set(double x, double y, double z, double angle);
    void set(const Vector3f& vec1, const Vector3f& vec2);
    void set(const Quaternion& quat);
    void set(const Rotation& rot);
    void setAxis(double x, double y, double z);
    void setAxis(const Vector3f& vec);
    void setAngle(double angle);

    friend bool operator!=(const Rotation& rot1, const Rotation& rot2);
    friend bool operator==(const Rotation& rot1, const Rotation& rot2);

    double x, y, z, angle;
};

inline void Rotation::set(double x, double y, double z, double angle)
{
    this->x = x; this->y = y; this->z = z; this->angle = angle;
}

inline void Rotation::set(const Rotation& rot)
{
    this->x = rot.x; this->y = rot.y; this->z = rot.z; this->angle = rot.angle;
}

inline void Rotation::setAxis(double x, double y, double z)
{
    this->x = x; this->y = y; this->z = z;
}

inline void Rotation::setAxis(const Vector3f& vec)
{
    this->x = vec.x; this->y = vec.y; this->z = vec.z;
}

inline void Rotation::setAngle(double angle)
{
    this->angle = angle;
}

inline bool operator==(const Rotation& rot1, const Rotation& rot2)
{
    return ((rot1.x == rot2.x) && (rot1.y == rot2.y) && (rot1.z == rot2.z) && (rot1.angle == rot2.angle));
}

inline bool operator!=(const Rotation& rot1, const Rotation& rot2)
{
    return ((rot1.x != rot2.x) || (rot1.y != rot2.y) || (rot1.z != rot2.z) || (rot1.angle != rot2.angle));
}

}

#endif
