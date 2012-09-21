/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Vector4.h: Vector4 class
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
#ifndef VECTOR4_H 
#define VECTOR4_H

#include <vector>

#include "Trace.h"

class Vector4  
{
public:
    float x, y, z, w;

    Vector4()
    {}

    Vector4(float x_val, float y_val, float z_val, float w_val)
    {
        set(x_val, y_val, z_val, w_val);
    }

    void set(float x_val, float y_val, float z_val, float w_val)
    {
	x = x_val, y = y_val, z = z_val, w = w_val;
    }

    void perspectiveDivide()
    {
        /* Divide vector by w */
        x /= w, y /= w, z /= w, w = 1.;
    }

    void print() const
    {
        TRACE("Vector4: (%.3f, %.3f, %.3f, %.3f)\n", x, y, z, w);
    }

    Vector4 operator +(const Vector4& op2) const
    {
        return Vector4(x + op2.x, y + op2.y, z + op2.z, w + op2.w);
    }

    Vector4 operator -(const Vector4& op2) const
    {
        return Vector4(x - op2.x, y - op2.y, z - op2.z, w - op2.w);
    }

    float operator *(const Vector4& op2) const
    {
        return (x * op2.x) + (y * op2.y) + (z * op2.z) + (w * op2.w);
    }

    Vector4 operator *(float op2) const
    {
        return Vector4(x * op2, y * op2, z * op2, w * op2);
    }

    Vector4 operator /(float op2) const
    {
        return Vector4(x / op2, y / op2, z / op2, w / op2);
    }
};

typedef std::vector<Vector4> Vector4Array;

#endif
