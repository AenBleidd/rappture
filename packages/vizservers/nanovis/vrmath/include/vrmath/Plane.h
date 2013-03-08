/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/** \class vrPlane vrPlane.h <vrmath/vrPlane.h>
 *  \author Insoo Woo(iwoo@purdue.edu), Sung-Ye Kim (inside@purdue.edu)
 *  \author PhD research assistants in PURPL at Purdue University  
 *  \version 1.0
 *  \date    Nov. 2006-2007
 */
#ifndef VRPLANE_H
#define VRPLANE_H

#include <vrmath/Vector4f.h>
#include <vrmath/Vector3f.h>

class vrMatrix4x4f;

class vrPlane
{
public:
    bool intersect(const vrVector3f& p1, const vrVector3f& p2, vrVector3f& intersectPoint) const;
    bool intersect(const vrVector3f& p1, const vrVector3f& p2, vrVector4f& intersectPoint) const;
    void transform(vrMatrix4x4f& mat);

    /// normal vector
    vrVector3f normal;

    /// @brief the distance from the origin
    float distance;
};

inline bool vrPlane::intersect(const vrVector3f& p1, const vrVector3f& p2, vrVector3f& intersectPoint) const
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

inline bool vrPlane::intersect(const vrVector3f& p1, const vrVector3f& p2, vrVector4f& intersectPoint) const
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

#endif
