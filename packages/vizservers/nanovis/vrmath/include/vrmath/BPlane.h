/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 * Author: Sung-Ye Kim <inside@purdue.edu>
 */
#ifndef VRBPLANE_H
#define VRBPLANE_H

#include <vrmath/Vector3f.h>

namespace vrmath {

class LineSegment;

class BPlane
{
public:
    void makePts(const Vector3f& pt1, const Vector3f& pt2, const Vector3f& pt3);

    void makeNormalPt(const Vector3f& norm, const Vector3f& pos);

    bool intersect(const LineSegment &seg, float &d) const;

    double distance(const Vector3f& point) const;

    Vector3f crossPoint(const Vector3f& point) const;

    Vector3f normal;
    Vector3f point;
};

}

#endif
