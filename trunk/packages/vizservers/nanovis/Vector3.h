/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Vector3.h : Vector class with 3 components
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef VECTOR3_H
#define VECTOR3_H

#include <math.h>

#include <vector>

#include "Mat4x4.h"

class Vector3
{
public:
    Vector3()
    {}

    Vector3(float x_val, float y_val, float z_val)
    {
        set(x_val, y_val, z_val);
    }

    void set(float x_val, float y_val, float z_val)
    {
        x = x_val, y = y_val, z = z_val;
    }

    void print() const
    {
        TRACE("(x:%f, y:%f, z:%f)", x, y, z);
    }

    bool operator==(const Vector3& op2) const
    {
        return (x == op2.x && y == op2.y && z == op2.z);
    }

    Vector3 operator -() const
    {
        return Vector3(-x, -y, -z);
    }

    Vector3 operator +(float scalar) const
    {
        return Vector3(x + scalar, y + scalar, z + scalar);
    }

    Vector3 operator -(float scalar) const
    {
        return Vector3(x - scalar, y - scalar, z - scalar);
    }

    Vector3 operator *(float scalar) const
    {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    Vector3 operator /(float scalar) const
    {
        return Vector3(x / scalar, y / scalar, z / scalar);
    }

    Vector3 operator +(const Vector3& op2) const
    {
        return Vector3(x + op2.x, y + op2.y, z + op2.z);
    }

    Vector3 operator -(const Vector3& op2) const
    {
        return Vector3(x - op2.x, y - op2.y, z - op2.z);
    }

    float dot(const Vector3& op2) const
    {
        return x*op2.x + y*op2.y + z*op2.z;
    }

    float operator *(const Vector3& op2) const
    {
        return dot(op2);
    }

    Vector3 cross(const Vector3& op2) const
    {
        return Vector3(y*op2.z - z*op2.y, z*op2.x - x*op2.z, x*op2.y - y*op2.x);
    }

    Vector3 operator ^(const Vector3& op2) const
    {
        return cross(op2);
    }

    Vector3 normalize() const
    {
        float len = length();
        if (len > 1.0e-6) {
            return Vector3(x / len, y / len, z / len);
        } else {
            return *this;
        }
    }

    Vector3 rotX(float degree) const
    {
        float rad = radians(degree);
        return Vector3(x,
                       y*cos(rad) - z*sin(rad),
                       y*sin(rad) + z*cos(rad));
    }

    Vector3 rotY(float degree) const
    {
        float rad = radians(degree);
        return Vector3(x*cos(rad) + z*sin(rad),
                       y,
                       -x*sin(rad) + z*cos(rad));
    }

    Vector3 rotZ(float degree) const
    {
        float rad = radians(degree);
        return Vector3(x*cos(rad) - y*sin(rad),
                       x*sin(rad) + y*cos(rad),
                       z);
    }

    float distance(const Vector3& v) const
    {
        return sqrtf(distanceSquare(v));
    }

    float distanceSquare(const Vector3& v) const
    {
        return (x-v.x)*(x-v.x) + (y-v.y)*(y-v.y) + (z-v.z)*(z-v.z);
    }

    float distanceSquare(float vx, float vy, float vz) const
    {
        return (x-vx)*(x-vx) + (y-vy)*(y-vy) + (z-vz)*(z-vz);
    }

    void transform(const Vector3& v, const Mat4x4& mat)
    {
	const float *m = mat.m;
	x = m[0] * v.x + m[4] * v.y + m[8]  * v.z + m[12];
	y = m[1] * v.x + m[5] * v.y + m[9]  * v.z + m[13];
	z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
    }

    float length() const
    {
        return sqrt(x*x + y*y + z*z);
    }

    Vector3 scale(const Vector3& scale) const
    {
        Vector3 v;
        v.x = x * scale.x;
        v.y = y * scale.y;
        v.z = z * scale.z;
        return v;
    }

    float x, y, z;

private:
    float radians(float degree) const
    {
        return (M_PI * degree) / 180.0;
    }
};

typedef std::vector<Vector3> Vector3Array;

/**
 * \brief Linear interpolation of 2 vectors
 */
inline Vector3 vlerp(const Vector3& v1, const Vector3& v2, double t)
{
    return Vector3(v1.x * (1.0-t) + v2.x * t,
                   v1.y * (1.0-t) + v2.y * t,
                   v1.z * (1.0-t) + v2.z * t);
}

#endif
