/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */

#include <vrmath/BPlane.h>
#include <vrmath/Vector3f.h>
#include <vrmath/LineSegment.h>

using namespace vrmath;

void BPlane::makePts(const Vector3f& p1, const Vector3f& p2, const Vector3f& p3)
{
    normal = cross(p2 - p1, p3 - p1);
    normal = normal.normalize();

    point = p1;
}

void BPlane::makeNormalPt(const Vector3f& norm, const Vector3f& pos)
{
    normal = norm;
    point = pos;
}

bool BPlane::intersect(const LineSegment &seg, float &d) const 
{
    float tu, td;

    tu = normal.x * (point.x - seg.pos.x) +
         normal.y * (point.y - seg.pos.y) +
         normal.z * (point.z - seg.pos.z);

    td = normal.x * seg.dir.x +
         normal.y * seg.dir.y +
         normal.z * seg.dir.z;

    if ( td == 0.0f ) return false;

    d = tu / td;

    return true;
}

double BPlane::distance(const Vector3f& pos) const 
{
    Vector3f plane_point = crossPoint(pos);

    return pos.distance(plane_point);
}

Vector3f BPlane::crossPoint(const Vector3f& pos) const
{
    LineSegment seg;

    seg.pos.set(pos.x, pos.y, pos.z, 1);
    seg.dir = normal;

    float t = 0;
    intersect(seg, t);

    return seg.getPoint(t);
}
