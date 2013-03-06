/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vrmath/Plane2.h>

void vrPlane2::makePts(const vrVector3f& p1, const vrVector3f& p2, const vrVector3f& p3)
{
    normal.cross(p2 - p1, p3 - p1);
    normal.normalize();

    point = p1;
}

void vrPlane2::makeNormalPt(const vrVector3f& norm, const vrVector3f& pos)
{
    normal = norm;
    point = pos;
}

bool vrPlane2::intersect(const vrLineSegment &seg, float &d) const 
{
    // STEVE
    // 2005-09-07
	
    float tu, td;

    tu = normal.x * (point.x - seg.pos.x) + normal.y * (point.y - seg.pos.y) 
        + normal.z * (point.z - seg.pos.z);

    td = normal.x * seg.dir.x + normal.y * seg.dir.y + normal.z * seg.dir.z;

    if ( td == 0.0f ) return false;

    d = tu / td;
	
    return true;
}

float vrPlane2::distance(const vrVector3f& pos) const 
{
    vrVector3f plane_point = crossPoint( pos );

    return pos.distance( plane_point );
}

vrVector3f vrPlane2::crossPoint(const vrVector3f& pos) const
{
    vrLineSegment seg;

    seg.pos = pos;
    seg.dir = normal;

    float t = 0;
    intersect(seg, t);

    return seg.getPoint(t);
}
