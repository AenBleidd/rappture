/*
 * ----------------------------------------------------------------------
 *  Rappture::FieldRect3D
 *    This is a continuous, linear function defined by a series of
 *    points on a 3D structured mesh.  It's a scalar field defined
 *    in 3D space.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RAPPTURE_FIELDRECT3D_H
#define RAPPTURE_FIELDRECT3D_H

#include "RpPtr.h"
#include "RpMeshRect3D.h"

namespace Rappture {

class FieldRect3D {
public:
    FieldRect3D();
    FieldRect3D(const Mesh1D& xg, const Mesh1D& yg, const Mesh1D& zg);
    FieldRect3D(const FieldRect3D& field);
    FieldRect3D& operator=(const FieldRect3D& field);
    virtual ~FieldRect3D();

    virtual int size(Axis which) const;
    virtual Node1D& nodeAt(Axis which, int pos);
    virtual double rangeMin(Axis which) const;
    virtual double rangeMax(Axis which) const;

    virtual FieldRect3D& define(int nodeId, double f);
    virtual double value(double x, double y, double z) const;

protected:
    virtual double _interpolate(double x0, double y0, double x1, double y1,
        double x) const;

private:
    std::deque<double> _valuelist;  // list of all values, in nodeId order
    Ptr<MeshRect3D> _meshPtr;       // mesh for all (x,y,z) points
    int _counter;                   // counter for generating node IDs
};

} // namespace Rappture

#endif
