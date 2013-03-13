/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */

#include <vrmath/LineSegment.h>
#include <vrmath/Matrix4x4d.h>

using namespace vrmath;

LineSegment::LineSegment():
    pos(0, 0, 0, 1), dir(0.0f, 0.0f, -1.0f), length(0.0f)
{
}

void LineSegment::transform(const Matrix4x4d &mat, const LineSegment &seg)
{
    pos = mat.transform(seg.pos);

    dir *= length;

    dir = mat.transformVec(seg.dir);
    length = dir.length();
    dir = dir.normalize();
}
