/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 * Author: Sung-Ye Kim <inside@purdue.edu>
 */
#ifndef VRLINESEGMENT_H
#define VRLINESEGMENT_H

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>

namespace vrmath {

class Matrix4x4d;

class LineSegment
{
public:
    LineSegment();

    /// Return the point
    Vector3f getPoint(float d) const;

    /// Transfrom the line segment using mat
    void transform(const Matrix4x4d &transMat, const LineSegment &seg);

    /// The position of the line segment
    Vector4f pos;

    /// The direction of the line segment
    Vector3f dir;

    /// The length of the line segment
    float length;
};

inline Vector3f LineSegment::getPoint(float d) const 
{
    return Vector3f(pos.x + d * dir.x, pos.y + d * dir.y, pos.z + d * dir.z);
}

}

#endif
