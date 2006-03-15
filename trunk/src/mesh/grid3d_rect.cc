#include "grid3d_rect.h"

RP_ERROR
RpGrid3dRect::addAllPoints(DataValType* xpts, int xdim, 
			   DataValType* ypts, int ydim,
			   DataValType* zpts, int zdim)
{
	if (xpts == NULL || ypts == NULL || zpts == NULL) {
		RpAppendErr("RpGrid3dRect::addAllPoints: null pointers");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	m_xpts.clear();
	m_ypts.clear();
	m_zpts.clear();

	copyArray(xpts, xdim, m_xpts);
	copyArray(ypts, ydim, m_ypts);
	copyArray(zpts, zdim, m_zpts);

	expandData();

	return RP_SUCCESS;

}

//
// add all points on x axis
//
RP_ERROR
RpGrid3dRect::addAllPointsX(DataValType* pts, int dim)
{
	if (pts == NULL) {
		RpAppendErr("RpGrid3dRect::addAllPointsX: null pointer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	copyArray(pts, dim, m_xpts);

	return RP_SUCCESS;
}

RP_ERROR
RpGrid3dRect::addAllPointsY(DataValType* pts, int dim)
{
	if (pts == NULL) {
		RpAppendErr("RpGrid3dRect::addAllPointsY: null pointer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	copyArray(pts, dim, m_ypts);

	return RP_SUCCESS;
}

RP_ERROR
RpGrid3dRect::addAllPointsZ(DataValType* pts, int dim)
{
	if (pts == NULL) {
		RpAppendErr("RpGrid3dRect::addAllPointsZ: null pointer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	copyArray(pts, dim, m_zpts);

	return RP_SUCCESS;
}

void
RpGrid3dRect::expandData()
{
if (m_data.size() != (unsigned) numPoints() ) {
	m_data.clear();

	for (int i=0; i < (signed)m_xpts.size(); i++) {
		for (int j=0;  j < (signed)m_ypts.size(); j++) {
		    for (int k=0; k < (signed)m_zpts.size(); k++) {
			m_data.push_back(m_xpts.at(i));
			m_data.push_back(m_ypts.at(j));
			m_data.push_back(m_zpts.at(k));
		    }
		}
	}
}
}

char * 
RpGrid3dRect::serialize(int& nbytes)
{
	expandData();

	return RpGrid3d::serialize(nbytes);
}

RP_ERROR
RpGrid3dRect::doSerialize(char* buf, int nbytes)
{
	expandData();

	return RpGrid3d::doSerialize(buf, nbytes);
}

//
int
RpGrid3dRect::numBytes()
{
	expandData();
	return RpGrid3d::numBytes();
}

