/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRVECTOR2F_H
#define VRVECTOR2F_H

#include <vrmath/vrLinmath.h>

#include <cstdlib>
#include <cmath>

class LmExport vrVector2f
{
public:
    vrVector2f() :
        x(0.0f), y(0.0f)
    {}

    vrVector2f(const vrVector2f& v) :
        x(v.x), y(v.y)
    {}

    vrVector2f(float x1, float y1) :
        x(x1), y(y1)
    {}

    void set(float x1, float y1);

    void set(const vrVector2f& v);

    float dot() const;

    float length() const;

    float distance(const vrVector2f& v) const;

    float distance(float x1, float y1) const;

    float distanceSquare(const vrVector2f& v) const;

    float distanceSquare(float x1, float y1) const;

    float x, y;
};

inline void vrVector2f::set(float x1, float y1)
{
    x = x1;
    y = y1;
}

inline void vrVector2f::set(const vrVector2f& v)
{
    x = v.x;
    y = v.y;
}

inline float vrVector2f::dot() const
{
    return (x * x + y * y);
}

inline float vrVector2f::length() const
{
    return sqrt(x * x + y * y);
}

inline float vrVector2f::distance(const vrVector2f& v) const
{
    float x1 = (v.x - x) , y1 = (v.y - y);
    return sqrt(x1 * x1 + y1 * y1);
}

inline float vrVector2f::distance(float x1, float y1) const
{	
    float x2 = (x1 - x) , y2 = (y1 - y);
    return sqrt(x2 * x2 + y2 * y2);
}

inline float vrVector2f::distanceSquare(const vrVector2f& v) const
{	
    float x1 = (v.x - x) , y1 = (v.y - y);
    return (x1 * x1 + y1 * y1);
}

inline float vrVector2f::distanceSquare(float x1, float y1) const
{	
    float x2 = (x1 - x) , y2 = (y1 - y);
    return (x2 * x2 + y2 * y2);
}

#endif
