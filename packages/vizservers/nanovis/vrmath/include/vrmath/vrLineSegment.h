/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/** \class vrPlane vrPlane <R2/math/vrPlane.h>
 *  \brief plane
 *  \author Insoo Woo(iwoo@purdue.edu), Sung-Ye Kim (inside@purdue.edu)
 *  \author PhD research assistants in PURPL at Purdue University  
 *  \version 1.0
 *  \date    Nov. 2006-2007
 */
#pragma once

#include <vrmath/vrLinmath.h>
#include <vrmath/vrVector3f.h>
#include <vrmath/vrMatrix4x4f.h>

class LmExport vrLineSegment {
public:
	/**
	 * @brief The position of the line segment
	 */
	vrVector3f pos;

	/**
	 * @brief The direction of the line segment
	 */
	vrVector3f dir;

	/**
	 * @brief The length of the line segment
	 */
	float length;

public :			
	/**
	 * @brief Constructor
	 */
	vrLineSegment();
	
	/**
	 * @brief Return the point 
	 */
	vrVector3f getPoint(float d) const;

	/**
	 * @brief Transfrom the line segment using mat
	 */
	void transform(const vrMatrix4x4f &transMat, const vrLineSegment &seg);
};

inline vrVector3f vrLineSegment::getPoint(float d) const 
{

	return vrVector3f(pos.x + d * dir.x, pos.y + d * dir.y, pos.z + d * dir.z);
}
