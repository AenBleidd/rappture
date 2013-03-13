/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 * Author: Sung-Ye Kim <inside@purdue.edu>
 */
#ifndef VRPLANE_H
#define VRPLANE_H

#include <vrmath/Vector4f.h>
#include <vrmath/Vector3f.h>

namespace vrmath {

class Matrix4x4f;

class Plane
{
public:
    bool intersect(const Vector3f& p1, const Vector3f& p2, Vector3f& intersectPoint) const;
    bool intersect(const Vector3f& p1, const Vector3f& p2, Vector4f& intersectPoint) const;
    void transform(Matrix4x4f& mat);

    /// normal vector
    Vector3f normal;

    /// @brief the distance from the origin
    float distance;
};

inline bool Plane::intersect(const Vector3f& p1, const Vector3f& p2, Vector3f& intersectPoint) const
{
    // http://astronomy.swin.edu.au/pbourke/geometry/planeline/
    float numerator = normal.x * p1.x + normal.y * p1.y + normal.z * p1.z;
    float denominator = normal.x * (p1.x - p2.x) + normal.y * (p1.y - p2.y) + normal.z * (p1.z - p2.z);

    if (denominator == 0.0f) return false;

    float u = numerator / denominator;
    if ((u > 0) && (u < 1.0f)) {
        return true;
    }

    intersectPoint.x = p1.x + u * (p2.x - p1.x);
    intersectPoint.y = p1.y + u * (p2.y - p1.y);
    intersectPoint.z = p1.z + u * (p2.z - p1.z);

    return false;
}

inline bool Plane::intersect(const Vector3f& p1, const Vector3f& p2, Vector4f& intersectPoint) const
{
    // http://astronomy.swin.edu.au/pbourke/geometry/planeline/
    float numerator = normal.x * p1.x + normal.y * p1.y + normal.z * p1.z;
    float denominator = normal.x * (p1.x - p2.x) + normal.y * (p1.y - p2.y) + normal.z * (p1.z - p2.z);
    if (denominator == 0.0f) return false;
    float u = numerator / denominator;

    if ((u > 0) && (u < 1.0f)) {
        return true;
    }

    intersectPoint.x = p1.x + u * (p2.x - p1.x);
    intersectPoint.y = p1.y + u * (p2.y - p1.y);
    intersectPoint.z = p1.z + u * (p2.z - p1.z);
    intersectPoint.w = 1.0f;

    return false;
}

}

#endif
