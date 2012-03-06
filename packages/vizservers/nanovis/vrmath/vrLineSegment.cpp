/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vrmath/vrLineSegment.h>


vrLineSegment::vrLineSegment() 
: pos(0.0f, 0.0f, 0.0f), dir(0.0f, 0.0f, -1.0f), length(0.0f)
{
	
}

void vrLineSegment::transform(const vrMatrix4x4f &mat, const vrLineSegment &seg)
{
	pos.transform(mat, seg.pos);
	
	dir.x *= length;
	dir.y *= length;
	dir.z *= length;

	dir.transformVec(mat, seg.dir);
	length = dir.length();
	dir.normalize();
}
