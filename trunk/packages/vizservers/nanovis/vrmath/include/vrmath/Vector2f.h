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

    Vector2f& operator=(const Vector2f& other)
    {
        if (&other != this) {
            set(other.x, other.y);
        }
        return *this;
    }

    void set(float x, float y);

    void set(const Vector2f& v);

    double dot() const;

    double length() const;

    double distance(const Vector2f& v) const;

    double distance(float x, float y) const;

    double distanceSquare(const Vector2f& v) const;

    double distanceSquare(float x, float y) const;

    float x, y;
};

inline void Vector2f::set(float x, float y)
{
    this->x = x;
    this->y = y;
}

inline void Vector2f::set(const Vector2f& v)
{
    x = v.x;
    y = v.y;
}

inline double Vector2f::dot() const
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

inline double Vector2f::distance(float x, float y) const
{	
    double x1 = ((double)x - (double)this->x) , y1 = ((double)y - (double)this->y);
    return sqrt(x1 * x1 + y1 * y1);
}

inline double Vector2f::distanceSquare(const Vector2f& v) const
{	
    double x1 = ((double)v.x - (double)x) , y1 = ((double)v.y - (double)y);
    return (x1 * x1 + y1 * y1);
}

inline double Vector2f::distanceSquare(float x, float y) const
{	
    double x1 = ((double)x - (double)this->x) , y1 = ((double)y - (double)this->y);
    return (x1 * x1 + y1 * y1);
}

}

#endif
