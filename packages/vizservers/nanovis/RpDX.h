/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 *  Rappture::DX
 *
 *  Rappture DX object for file reading and interacting
 *  with libDX and friends.
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005-2008  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RAPPTURE_DX_H
#define RAPPTURE_DX_H

#include <dx/dx.h>
#include <RpOutcome.h>

namespace Rappture {

class DX {
public:
    DX();
    DX(Rappture::Outcome &result, const char* filename);
    DX(const DX& rpdx);
    DX& operator=(const DX& rpdx);
    virtual ~DX();

    /*
    virtual double value(double x, double y, double z,
        double outside=NAN) const;
    virtual double valueMin() const;
    virtual double valueMax() const;
    */
    virtual DX& interpolate(int* newAxisLen);

    int n(void) const {
	return _n;
    }
    int rank(void) const {
	return _rank;
    }
    int shape(void) const {
	return _shape;
    }
    const float* delta(void) const {
	// FIXME: Delta is always three numbers.
	return _delta;
    }
    const float *max(void) const {
	return _max;
    }
    const float *origin(void) const {
	return _origin;
    }
    const float * positions(void) const {
	return _positions;
    }
    const int *axisLen(void) const {
	return _axisLen;
    }
    const float *data(void) const {
	return _data;
    }
    float dataMin(void) const {
	return _dataMin;
    }
    float dataMax(void) const {
	return _dataMax;
    }
    float nzero_min(void) const {
	return _nzero_min;
    }

protected:

private:
    float _dataMin;
    float _dataMax;
    float _nzero_min;
    int _numAxis;       // same as _shape if _rank == 1
    int *_axisLen;      // number of points on each axis
    float* _data;

    int _n;             // number of points in the position array
    int _rank;          // number of dimensions in each item
    int _shape;         // array of the extents of each dimension
    float* _positions;  // array holding the x,y,z coord of each point
    float* _delta;      // array holding deltas of the uniform mesh
    float* _max;        // array hodling coord of most distant pt from origin
    float* _origin;     // array holding coord of origin

    Object _dxobj;

    void __findPosMax();
    void __collectDataStats();
    void __getInterpPos();
    void __getInterpData();


};

} // namespace Rappture

#endif
