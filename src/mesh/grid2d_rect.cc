#include "grid2d_rect.h"

RP_ERROR
RpGrid2dRect::addAllPoints(DataValType* xpts, int xdim, 
			   DataValType* ypts, int ydim)
{
	if (xpts == NULL || ypts == NULL) {
		RpAppendErr("RpGrid2dRect::addAllPoints: null pointers");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	m_xpts.clear();
	m_ypts.clear();

	copyArray(xpts, xdim, m_xpts);
	copyArray(ypts, ydim, m_ypts);

	expandData();

	return RP_SUCCESS;

}

//
// add all points on x axis
//
RP_ERROR
RpGrid2dRect::addAllPointsX(DataValType* pts, int dim)
{
	if (pts == NULL) {
		RpAppendErr("RpGrid2dRect::addAllPointsX: null pointer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	copyArray(pts, dim, m_xpts);

	return RP_SUCCESS;
}

RP_ERROR
RpGrid2dRect::addAllPointsY(DataValType* pts, int dim)
{
	if (pts == NULL) {
		RpAppendErr("RpGrid2dRect::addAllPointsY: null pointer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	copyArray(pts, dim, m_ypts);

	return RP_SUCCESS;
}

void
RpGrid2dRect::expandData()
{
if (m_data.size() != (unsigned) numPoints() ) {
	m_data.clear();

	for (int i=0; i < (signed)m_xpts.size(); i++) {
		for (int j=0;  j < (signed)m_ypts.size(); j++) {
			m_data.push_back(m_xpts.at(i));
			m_data.push_back(m_ypts.at(j));
		}
	}
}
}

char * 
RpGrid2dRect::serialize(int& nbytes)
{
	expandData();

	return RpGrid2d::serialize(nbytes);
}

RP_ERROR
RpGrid2dRect::doSerialize(char* buf, int nbytes)
{
	expandData();

	return RpGrid2d::doSerialize(buf, nbytes);
}

// must expand data before calculating number of bytes
int
RpGrid2dRect::numBytes()
{
	expandData();
	return RpGrid2d::numBytes();
}

