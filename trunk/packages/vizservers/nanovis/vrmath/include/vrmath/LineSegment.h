/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/** \class vrLineSegment vrLineSegment.h <vrmath/vrLineSegment.h>
 *  \author Insoo Woo(iwoo@purdue.edu), Sung-Ye Kim (inside@purdue.edu)
 *  \author PhD research assistants in PURPL at Purdue University  
 *  \version 1.0
 *  \date    Nov. 2006-2007
 */
#ifndef VRLINESEGMENT_H
#define VRLINESEGMENT_H

#include <vrmath/Linmath.h>
#include <vrmath/Vector3f.h>
#include <vrmath/Matrix4x4f.h>

class vrLineSegment
{
public:
    vrLineSegment();

    /// Return the point
    vrVector3f getPoint(float d) const;

    /// Transfrom the line segment using mat
    void transform(const vrMatrix4x4f &transMat, const vrLineSegment &seg);

    /// The position of the line segment
    vrVector3f pos;

    /// The direction of the line segment
    vrVector3f dir;

    /// The length of the line segment
    float length;
};

inline vrVector3f vrLineSegment::getPoint(float d) const 
{
    return vrVector3f(pos.x + d * dir.x, pos.y + d * dir.y, pos.z + d * dir.z);
}

#endif
