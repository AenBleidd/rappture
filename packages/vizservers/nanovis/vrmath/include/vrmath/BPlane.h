/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/** \class vrBPlane vrBPlane.h <vrmath/vrBPlane.h>
 *  \author Insoo Woo(iwoo@purdue.edu), Sung-Ye Kim (inside@purdue.edu)
 *  \author PhD research assistants in PURPL at Purdue University  
 *  \version 1.0
 *  \date    Nov. 2006-2007
 */
#ifndef VRBPLANE_H
#define VRBPLANE_H

#include <vrmath/Linmath.h>
#include <vrmath/Vector3f.h>
#include <vrmath/LineSegment.h>

class vrBPlane
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
