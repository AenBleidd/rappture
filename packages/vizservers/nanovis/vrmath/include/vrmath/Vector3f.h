/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRVECTOR3F_H
#define VRVECTOR3F_H

#include <cstdlib>
#include <cmath>

#include <vrmath/Linmath.h>

class vrMatrix4x4f;

class vrVector3f
{
public:
    vrVector3f() :
        x(0.0f), y(0.0f), z(0.0f)
    {}

    vrVector3f(const vrVector3f& v3) :
        x(v3.x), y(v3.y), z(v3.z)
    {}

    vrVector3f(float x1, float y1, float z1) :
        x(x1), y(y1), z(z1)
    {}

    void set(float x1, float y1, float z1);

    void set(const vrVector3f& v3);

    float dot(const vrVector3f& v) const;

    float length() const;

    float distance(const vrVector3f& v3) const;

    float distance(float x1, float y1, float z1) const;

    float distanceSquare(const vrVector3f& v3) const;

    float distanceSquare(float x1, float y1, float z1) const;

    void cross(const vrVector3f& v1, const vrVector3f& v2);

    void cross(const vrVector3f& v1);

    void transform(const vrMatrix4x4f& m, const vrVector3f& v);

    void transformVec(const vrMatrix4x4f& m, const vrVector3f& v);

    void scale(vrVector3f& sc);

    float normalize();

    void scale(float scale);

    bool operator==(const vrVector3f& v) const;

    bool operator!=(const vrVector3f& v) const;

    vrVector3f& operator+=(const vrVector3f& value1);
    vrVector3f operator*(const vrVector3f& scale); // scale
    vrVector3f operator*(float scale);
    vrVector3f operator-(void) const ;

    bool isEqual(const vrVector3f& v) const;
    bool isAlmostEqual(const vrVector3f& v, float tol) const;

    float getX() const;
    float getY() const;
    float getZ() const;

    friend vrVector3f operator-(const vrVector3f& value1, const vrVector3f& value2);
    friend vrVector3f operator+(const vrVector3f& value1, const vrVector3f& value2);
    friend vrVector3f operator*(const vrVector3f& value, float scale);

    float x, y, z;
};


inline bool vrVector3f::operator==(const vrVector3f &v) const 
{
    return ( v.x == x && v.y == y && v.z == z);
}

inline bool vrVector3f::operator!=(const vrVector3f &v) const 
{
    return !(v.x == x && v.y == y && v.z == z);
}

inline vrVector3f vrVector3f::operator*(float scale)
{
    vrVector3f vec;
    vec.x = x * scale;
    vec.y = y * scale;
    vec.z = z * scale;
    return vec;
}

inline vrVector3f vrVector3f::operator*(const vrVector3f& scale)
{
    vrVector3f vec;
    vec.x = x * scale.x;
    vec.y = y * scale.y;
    vec.z = z * scale.z;
    return vec;
}

inline vrVector3f& vrVector3f::operator+=(const vrVector3f& scale)
{
    x = x + scale.x;
    y = y + scale.y;
    z = z + scale.z;
    return *this;
}

inline vrVector3f operator-(const vrVector3f& value1, const vrVector3f& value2)
{
    return vrVector3f(value1.x - value2.x, value1.y - value2.y, value1.z - value2.z);
}

inline vrVector3f operator+(const vrVector3f& value1, const vrVector3f& value2)
{
    return vrVector3f(value1.x + value2.x, value1.y + value2.y, value1.z + value2.z);
}

inline void vrVector3f::set(float x1, float y1, float z1)
{
    x = x1;
    y = y1;
    z = z1;
}

inline void vrVector3f::set(const vrVector3f& v3)
{
    x = v3.x;
    y = v3.y;
    z = v3.z;
}

inline float vrVector3f::dot(const vrVector3f& v) const
{
    return (x * v.x + y * v.y + z * v.z);
}

inline float vrVector3f::length() const
{
    return sqrt(x * x + y * y + z * z);
}

inline float vrVector3f::distance(const vrVector3f& v3) const
{
    float x1 = (v3.x - x) , y1 = (v3.y - y), z1 = (v3.z - z);
    return sqrt(x1 * x1 + y1 * y1 + z1 * z1);
}

inline float vrVector3f::distance(float x1, float y1, float z1) const
{	
    float x2 = (x1 - x) , y2 = (y1 - y), z2 = (z1 - z);
    return sqrt(x2 * x2 + y2 * y2 + z2 * z2);
}

inline float vrVector3f::distanceSquare(const vrVector3f& v3) const
{	
    float x1 = (v3.x - x) , y1 = (v3.y - y), z1 = (v3.z - z);
    return (x1 * x1 + y1 * y1 + z1 * z1);
}

inline float vrVector3f::distanceSquare(float x1, float y1, float z1) const
{	
    float x2 = (x1 - x) , y2 = (y1 - y), z2 = (z1 - z);
    return (x2 * x2 + y2 * y2 + z2 * z2);
}

inline void vrVector3f::cross(const vrVector3f& v1, const vrVector3f& v2)
{
    float vec[3];
    vec[0] = v1.y * v2.z - v2.y * v1.z;
    vec[1] = v1.z * v2.x - v2.z * v1.x;
    vec[2] = v1.x * v2.y - v2.x * v1.y;

    x = vec[0];
    y = vec[1];
    z = vec[2];
}

inline void vrVector3f::cross(const vrVector3f& v1)
{
    float vec[3];
    vec[0] = y * v1.z - v1.y * z;
    vec[1] = z * v1.x - v1.z * x;
    vec[2] = x * v1.y - v1.x * y;
    x = vec[0];
    y = vec[1];
    z = vec[2];
}

inline float vrVector3f::normalize()
{
    float inverseLength = length();
    if (inverseLength != 0) {
        inverseLength = 1 / inverseLength;
        x *= inverseLength;
        y *= inverseLength;
        z *= inverseLength;
    }

    return inverseLength;
}

inline void vrVector3f::scale(float scale)
{
    x *= scale;
    y *= scale;
    z *= scale;
}

inline void vrVector3f::scale(vrVector3f& sc)
{
    x *= sc.x;
    y *= sc.y;
    z *= sc.z;
}

inline vrVector3f operator*(const vrVector3f& value, float scale)
{
    return vrVector3f(value.x * scale, value.y * scale, value.z * scale);
}

inline bool vrVector3f::isAlmostEqual(const vrVector3f& v, float tol) const
{
    return (v.x - tol) <= x && x <= (v.x + tol) &&
        (v.y - tol) <= y && y <= (v.y + tol) &&
        (v.z - tol) <= z && z <= (v.z + tol);
}

inline bool vrVector3f::isEqual(const vrVector3f& v) const
{
    return ( v.x == x && v.y == y && v.z == z );
}

inline vrVector3f  vrVector3f::operator-() const
{
    return vrVector3f(-x, -y, -z);
}

inline float vrVector3f::getX() const
{
    return x;
}

inline float vrVector3f::getY() const
{
    return y;
}

inline float vrVector3f::getZ() const
{
    return z;
}

#endif
