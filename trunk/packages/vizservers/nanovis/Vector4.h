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
        TRACE("Vector4: (%.3f, %.3f, %.3f, %.3f)", x, y, z, w);
    }

    bool operator==(const Vector4& op2) const
    {
        return (x == op2.x && y == op2.y && z == op2.z && w == op2.w);
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

    float x, y, z, w;
};

typedef std::vector<Vector4> Vector4Array;

/**
 * \brief Linear interpolation of 2 points
 */
inline Vector4 vlerp(const Vector4& v1, const Vector4& v2, double t)
{
    return Vector4(v1.x * (1.0-t) + v2.x * t,
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
planeLineIntersection(const Vector4& pt1, const Vector4& pt2,
                      const Vector4& plane, Vector4& ret)
{
    float a = plane.x;
    float b = plane.y;
    float c = plane.z;
    float d = plane.w;

    Vector4 p1 = pt1;
    float x1 = p1.x;
    float y1 = p1.y;
    float z1 = p1.z;

    Vector4 p2 = pt2;
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
        TRACE("No intersection between line and plane");
        return false;
    }

    float u = uNumer / uDenom;
    ret.x = x1 + u * (x2 - x1);
    ret.y = y1 + u * (y2 - y1);
    ret.z = z1 + u * (z2 - z1);
    ret.w = 1;
    return true;
}

#endif
