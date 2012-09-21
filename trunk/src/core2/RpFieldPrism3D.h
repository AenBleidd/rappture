/*
 * ----------------------------------------------------------------------
 *  Rappture::FieldPrism3D
 *    This is a continuous, linear function defined by a series of
 *    points on a 3D prismatic mesh.  It's a scalar field defined
 *    in 3D space.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RPFIELDPRISM3D_H
#define RPFIELDPRISM3D_H

#include <math.h>
#include <vector>
#include <RpPtr.h>
#include <RpMeshPrism3D.h>

namespace Rappture {

class FieldPrism3D {
public:
    FieldPrism3D();
    FieldPrism3D(const MeshTri2D& xyg, const Mesh1D& zg);
    FieldPrism3D(const FieldPrism3D& field);
    FieldPrism3D& operator=(const FieldPrism3D& field);
    virtual ~FieldPrism3D();

    virtual double rangeMin(Axis which) const;
    virtual double rangeMax(Axis which) const;

    virtual FieldPrism3D& define(int nodeId, double f);
    virtual double value(double x, double y, double z,
        double outside=NAN) const;
    virtual double valueMin() const;
    virtual double valueMax() const;

private:
    std::vector<double> _valuelist;  // list of all values, in nodeId order
    double _vmin;                    // minimum value in _valuelist
    double _vmax;                    // maximum value in _valuelist
    Ptr<MeshPrism3D> _meshPtr;       // mesh for all (x,y,z) points
    int _counter;                    // counter for generating node IDs
};

} // namespace Rappture

#endif
