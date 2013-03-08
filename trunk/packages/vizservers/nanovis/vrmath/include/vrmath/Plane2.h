/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/** \class vrPlane2 vrPlane2.h <vrmath/vrPlane2.h>
 *  \author Insoo Woo(iwoo@purdue.edu), Sung-Ye Kim (inside@purdue.edu)
 *  \author PhD research assistants in PURPL at Purdue University  
 *  \version 1.0
 */
#ifndef VRPLANE2_H
#define VRPLANE2_H

#include <vrmath/Vector3f.h>

class vrLineSegment;

class vrPlane2
{
public:
    void makePts(const vrVector3f& pt1, const vrVector3f& pt2, const vrVector3f& pt3);

    void makeNormalPt(const vrVector3f& norm, const vrVector3f& pos);

    bool intersect(const vrLineSegment &seg, float &d) const;

    float distance(const vrVector3f& point) const;

    vrVector3f crossPoint(const vrVector3f& point) const;

    vrVector3f normal;
    vrVector3f point;
};

#endif
