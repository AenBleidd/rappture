/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */
#ifndef VRVECTOR4F_H
#define VRVECTOR4F_H

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>

namespace vrmath {

class Vector4f
{
public:
    Vector4f() :
        x(0.0f), y(0.0f), z(0.0f), w(0.0f)
    {}

    Vector4f(const Vector4f& other) :
        x(other.x), y(other.y), z(other.z), w(other.w)
    {}

    Vector4f(const Vector3f& v, float w1) :
        x(v.x), y(v.y), z(v.z), w(w1)
    {}

    Vector4f(float x1, float y1, float z1, float w1) :
        x(x1), y(y1), z(z1), w(w1)
    {}

    void set(float x, float y, float z, float w);
 
    void set(const Vector3f& v, float w);

    void set(const Vector4f& v);

    Vector4f& operator=(const Vector4f& other)
    {
        if (&other != this) {
            set(other.x, other.y, other.z, other.w);
        }
        return *this;
    }

    bool operator==(const Vector4f& op2) const
    {
        return (x == op2.x && y == op2.y && z == op2.z && w == op2.w);
    }

    Vector4f operator+(const Vector4f& op2) const
    {
        return Vector4f(x + op2.x, y + op2.y, z + op2.z, w + op2.w);
    }

    Vector4f operator-(const Vector4f& op2) const
    {
        return Vector4f(x - op2.x, y - op2.y, z - op2.z, w - op2.w);
    }

    float operator*(const Vector4f& op2) const
    {
        return (x * op2.x) + (y * op2.y) + (z * op2.z) + (w * op2.w);
    }

    Vector4f operator*(float op2) const
    {
        return Vector4f(x * op2, y * op2, z * op2, w * op2);
    }

    Vector4f operator/(float op2) const
    {
        return Vector4f(x / op2, y / op2, z / op2, w / op2);
    }

    void divideByW();

    float dot(const Vector4f& vec) const;

    union {
        struct {
            float x, y, z, w;
        };
        struct {
            float r, g, b, a;
        };
    };
};

inline void Vector4f::divideByW()
{
    if (w != 0) {
        x /= w; y /= w; z /= w;
        w = 1.0f;
    }
}

inline void Vector4f::set(float x, float y, float z, float w)
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

inline void Vector4f::set(const Vector4f& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    w = v.w;
}

inline void Vector4f::set(const Vector3f& v, float w)
{
    x = v.x;
    y = v.y;
    z = v.z;
    this->w = w;
}

inline float Vector4f::dot(const Vector4f& v) const
{
    return (x * v.x + y * v.y + z * v.z + w * v.w);
}

/**
 * \brief Linear interpolation of 2 points
 */
inline Vector4f vlerp(const Vector4f& v1, const Vector4f& v2, double t)
{
    return Vector4f(v1.x * (1.0-t) + v2.x * t,
                    v1.y * (1.0-t) + v2.y * t,
                    v1.z * (1.0-t) + v2.z * t,
                    v1.w * (1.0-t) + v2.w * t);
}

/**
 * \brief Finds the intersection of a line through 
 * p1 and p2 with the given plane.
 *
 * If the line lies in the plane, an arbitrary 
 * point on the line is returned.
 *
 * Reference:
 * http://astronomy.swin.edu.au/pbourke/geometry/planeline/
 *
 * \param[in] pt1 First point
 * \param[in] pt2 Second point
 * \param[in] plane Plane equation coefficients
 * \param[out] ret Point of intersection if a solution was found
 * \return Bool indicating if an intersection was found
 */
inline bool 
planeLineIntersection(const Vector4f& pt1, const Vector4f& pt2,
                      const Vector4f& plane, Vector4f& ret)
{
    float a = plane.x;
    float b = plane.y;
    float c = plane.z;
    float d = plane.w;

    Vector4f p1 = pt1;
    float x1 = p1.x;
    float y1 = p1.y;
    float z1 = p1.z;

    Vector4f p2 = pt2;
    float x2 = p2.x;
    float y2 = p2.y;
    float z2 = p2.z;

    float uDenom = a * (x1 - x2) + b * (y1 - y2) + c * (z1 - z2);
    float uNumer = a * x1 + b * y1 + c * z1 + d;

    if (uDenom == 0.0f) {
        // Plane parallel to line
        if (uNumer == 0.0f) {
            // Line within plane
            ret = p1;
            return true;
        }
        // No solution
        return false;
    }

    float u = uNumer / uDenom;
    ret.x = x1 + u * (x2 - x1);
    ret.y = y1 + u * (y2 - y1);
    ret.z = z1 + u * (z2 - z1);
    ret.w = 1;
    return true;
}

}

#endif
