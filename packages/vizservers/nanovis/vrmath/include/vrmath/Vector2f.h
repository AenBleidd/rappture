/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */
#ifndef VRVECTOR2F_H
#define VRVECTOR2F_H

#include <cstdlib>
#include <cmath>

namespace vrmath {

class Vector2f
{
public:
    Vector2f() :
        x(0.0f), y(0.0f)
    {}

    Vector2f(const Vector2f& v) :
        x(v.x), y(v.y)
    {}

    Vector2f(float x1, float y1) :
        x(x1), y(y1)
    {}

    void set(float x1, float y1);

    void set(const Vector2f& v);

    float dot() const;

    double length() const;

    double distance(const Vector2f& v) const;

    double distance(float x1, float y1) const;

    double distanceSquare(const Vector2f& v) const;

    double distanceSquare(float x1, float y1) const;

    float x, y;
};

inline void Vector2f::set(float x1, float y1)
{
    x = x1;
    y = y1;
}

inline void Vector2f::set(const Vector2f& v)
{
    x = v.x;
    y = v.y;
}

inline float Vector2f::dot() const
{
    return (x * x + y * y);
}

inline double Vector2f::length() const
{
    return sqrt(x * x + y * y);
}

inline double Vector2f::distance(const Vector2f& v) const
{
    double x1 = ((double)v.x - (double)x) , y1 = ((double)v.y - (double)y);
    return sqrt(x1 * x1 + y1 * y1);
}

inline double Vector2f::distance(float x1, float y1) const
{	
    double x2 = ((double)x1 - (double)x) , y2 = ((double)y1 - (double)y);
    return sqrt(x2 * x2 + y2 * y2);
}

inline double Vector2f::distanceSquare(const Vector2f& v) const
{	
    double x1 = ((double)v.x - (double)x) , y1 = ((double)v.y - (double)y);
    return (x1 * x1 + y1 * y1);
}

inline double Vector2f::distanceSquare(float x1, float y1) const
{	
    double x2 = ((double)x1 - (double)x) , y2 = ((double)y1 - (double)y);
    return (x2 * x2 + y2 * y2);
}

}

#endif
