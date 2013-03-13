/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */
#ifndef VRVECTOR3F_H
#define VRVECTOR3F_H

#include <cstdlib>
#include <cmath>

namespace vrmath {

class Vector3f
{
public:
    Vector3f() :
        x(0.0f), y(0.0f), z(0.0f)
    {}

    Vector3f(const Vector3f& v3) :
        x(v3.x), y(v3.y), z(v3.z)
    {}

    Vector3f(float x1, float y1, float z1) :
        x(x1), y(y1), z(z1)
    {}

    void set(float x1, float y1, float z1);

    void set(const Vector3f& v3);

    double length() const;

    double distance(const Vector3f& v3) const;

    double distance(float x1, float y1, float z1) const;

    double distanceSquare(const Vector3f& v3) const;

    double distanceSquare(float x1, float y1, float z1) const;

    double dot(const Vector3f& v) const;

    Vector3f cross(const Vector3f& v1) const;

    Vector3f normalize() const;

    Vector3f scale(Vector3f& sc) const;

    Vector3f scale(float scale) const;

    bool operator==(const Vector3f& v) const;

    bool operator!=(const Vector3f& v) const;

    Vector3f operator-() const;

    // scalar ops
    Vector3f operator+(float scalar) const;
    Vector3f operator-(float scalar) const;
    Vector3f operator*(float scalar) const;
    Vector3f operator/(float scalar) const;
    Vector3f& operator*=(float scalar);

    // vector ops
    Vector3f operator+(const Vector3f& op2) const;
    Vector3f operator-(const Vector3f& op2) const;
    Vector3f& operator+=(const Vector3f& op2);
    Vector3f& operator-=(const Vector3f& op2);

    bool isEqual(const Vector3f& v) const;
    bool isAlmostEqual(const Vector3f& v, float tol) const;

    float getX() const;
    float getY() const;
    float getZ() const;

    //friend Vector3f operator*(float scale, const Vector3f& value);

    float x, y, z;
};

inline bool Vector3f::operator==(const Vector3f &v) const 
{
    return (v.x == x && v.y == y && v.z == z);
}

inline bool Vector3f::operator!=(const Vector3f &v) const 
{
    return !(v.x == x && v.y == y && v.z == z);
}

inline Vector3f Vector3f::operator-() const
{
    return Vector3f(-x, -y, -z);
}

inline Vector3f Vector3f::operator+(float scalar) const
{
    return Vector3f(x + scalar, y + scalar, z + scalar);
}

inline Vector3f Vector3f::operator-(float scalar) const
{
    return Vector3f(x - scalar, y - scalar, z - scalar);
}

inline Vector3f Vector3f::operator*(float scalar) const
{
    return Vector3f(x * scalar, y * scalar, z * scalar);
}

inline Vector3f Vector3f::operator/(float scalar) const
{
    return Vector3f(x / scalar, y / scalar, z / scalar);
}

inline Vector3f& Vector3f::operator*=(float scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

inline Vector3f Vector3f::operator+(const Vector3f& op2) const
{
    return Vector3f(x + op2.x, y + op2.y, z + op2.z);
}

inline Vector3f Vector3f::operator-(const Vector3f& op2) const
{
    return Vector3f(x - op2.x, y - op2.y, z - op2.z);
}

inline Vector3f& Vector3f::operator+=(const Vector3f& op2)
{
    x += op2.x;
    y += op2.y;
    z += op2.z;
    return *this;
}

inline Vector3f& Vector3f::operator-=(const Vector3f& op2)
{
    x -= op2.x;
    y -= op2.y;
    z -= op2.z;
    return *this;
}

#if 0
inline Vector3f operator*(float scale, const Vector3f& value)
{
    return Vector3f(value.x * scale, value.y * scale, value.z * scale);
}
#endif

inline void Vector3f::set(float x1, float y1, float z1)
{
    x = x1;
    y = y1;
    z = z1;
}

inline void Vector3f::set(const Vector3f& v3)
{
    x = v3.x;
    y = v3.y;
    z = v3.z;
}

inline double Vector3f::dot(const Vector3f& v) const
{
    return ((double)x * v.x + (double)y * v.y + (double)z * v.z);
}

inline double Vector3f::length() const
{
    return sqrt((double)x * x + (double)y * y + (double)z * z);
}

inline double Vector3f::distance(const Vector3f& v3) const
{
    double x1 = ((double)v3.x - (double)x) , y1 = ((double)v3.y - (double)y), z1 = ((double)v3.z - (double)z);
    return sqrt(x1 * x1 + y1 * y1 + z1 * z1);
}

inline double Vector3f::distance(float x1, float y1, float z1) const
{	
    double x2 = ((double)x1 - (double)x) , y2 = ((double)y1 - (double)y), z2 = ((double)z1 - (double)z);
    return sqrt(x2 * x2 + y2 * y2 + z2 * z2);
}

inline double Vector3f::distanceSquare(const Vector3f& v3) const
{	
    double x1 = ((double)v3.x - (double)x) , y1 = ((double)v3.y - (double)y), z1 = ((double)v3.z - (double)z);
    return (x1 * x1 + y1 * y1 + z1 * z1);
}

inline double Vector3f::distanceSquare(float x1, float y1, float z1) const
{	
    double x2 = ((double)x1 - (double)x) , y2 = ((double)y1 - (double)y), z2 = ((double)z1 - (double)z);
    return (x2 * x2 + y2 * y2 + z2 * z2);
}

inline Vector3f Vector3f::cross(const Vector3f& op2) const
{
    return Vector3f(y * op2.z - z * op2.y,
                    z * op2.x - x * op2.z,
                    x * op2.y - y * op2.x);
}

inline Vector3f Vector3f::normalize() const
{
    float len = length();
    if (len > 1.0e-6) {
        return Vector3f(x / len, y / len, z / len);
    } else {
        return *this;
    }
}

inline Vector3f Vector3f::scale(float scale) const
{
    return Vector3f(x * scale, y * scale, z * scale);
}

inline Vector3f Vector3f::scale(Vector3f& sc) const
{
    return Vector3f(x * sc.x, y * sc.y, z * sc.z);
}

inline bool Vector3f::isAlmostEqual(const Vector3f& v, float tol) const
{
    return (v.x - tol) <= x && x <= (v.x + tol) &&
        (v.y - tol) <= y && y <= (v.y + tol) &&
        (v.z - tol) <= z && z <= (v.z + tol);
}

inline bool Vector3f::isEqual(const Vector3f& v) const
{
    return ( v.x == x && v.y == y && v.z == z );
}

inline float Vector3f::getX() const
{
    return x;
}

inline float Vector3f::getY() const
{
    return y;
}

inline float Vector3f::getZ() const
{
    return z;
}

inline Vector3f cross(const Vector3f& op1, const Vector3f& op2)
{
    return Vector3f(op1.cross(op2));
}

/**
 * \brief Linear interpolation of 2 vectors
 */
inline Vector3f vlerp(const Vector3f& v1, const Vector3f& v2, double t)
{
    return Vector3f(v1.x * (1.0-t) + v2.x * t,
                    v1.y * (1.0-t) + v2.y * t,
                    v1.z * (1.0-t) + v2.z * t);
}

}

#endif
