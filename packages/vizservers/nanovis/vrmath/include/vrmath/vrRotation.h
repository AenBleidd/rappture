/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRROTATION_H
#define VRROTATION_H

#include <vrmath/vrLinmath.h>

class vrVector3f;
class vrQuaternion;

class LmExport vrRotation
{
public:
    vrRotation() :
        x(0.0f), y(1.0f), z(0.0f), angle(0.0f)
    {}

    vrRotation(float x1, float y1, float z1, float angle1) :
        x(x1), y(y1), z(z1), angle(angle1)
    {}

    vrRotation(const vrRotation& rotation) :
        x(rotation.x), y(rotation.y), z(rotation.z), angle(rotation.angle)
    {}

    float getX() const;
    float getY() const;
    float getZ() const;
    float getAngle() const;

    void set(float x, float y, float z, float angle);
    void set(const vrVector3f &vec1, const vrVector3f &vec2);
    void set(const vrQuaternion& quat);
    void set(const vrRotation& rot);
    void setAxis(float x, float y, float z);
    void setAngle(float angle);
	
    friend bool operator!=(const vrRotation& rot1, const vrRotation& rot2);
    friend bool operator==(const vrRotation& rot1, const vrRotation& rot2);

    float x, y, z, angle;
};

inline float vrRotation::getX() const
{
    return x;
}

inline float vrRotation::getY() const
{
    return y;
}

inline float vrRotation::getZ() const
{
    return z;
}

inline float vrRotation::getAngle() const
{
    return angle;
}

inline void vrRotation::set(float x, float y, float z, float angle)
{
    this->x = x; this->y = y; this->z = z; this->angle = angle;
}

inline void vrRotation::set(const vrRotation& rot)
{
    this->x = rot.x; this->y = rot.y; this->z = rot.z; this->angle = rot.angle;
}

inline void vrRotation::setAxis(float x, float y, float z)
{
    this->x = x; this->y = y; this->z = z;
}

inline void vrRotation::setAngle(float angle)
{
    this->angle = angle;
}

inline bool operator==(const vrRotation& rot1, const vrRotation& rot2)
{
    return ((rot1.x == rot2.x) && (rot1.y == rot2.y) && (rot1.z == rot2.z) && (rot1.angle == rot2.angle));
}

inline bool operator!=(const vrRotation& rot1, const vrRotation& rot2)
{
    return ((rot1.x != rot2.x) || (rot1.y != rot2.y) || (rot1.z != rot2.z) || (rot1.angle != rot2.angle));
}

#endif
