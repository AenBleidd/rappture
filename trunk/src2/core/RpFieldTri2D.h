/*
 * ----------------------------------------------------------------------
 *  Rappture::FieldTri2D
 *    This is a continuous, linear function defined by a series of
 *    points on a 2D unstructured mesh.  It's a scalar field defined
 *    in 2D space.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RAPPTURE_FIELDTRI2D_H
#define RAPPTURE_FIELDTRI2D_H

#include <math.h>
#include <vector>
#include "RpPtr.h"
#include "RpMeshTri2D.h"

namespace Rappture {

class FieldTri2D {
public:
    FieldTri2D();
    FieldTri2D(const MeshTri2D& grid);
    FieldTri2D(const FieldTri2D& field);
    FieldTri2D& operator=(const FieldTri2D& field);
    virtual ~FieldTri2D();

    virtual int size() const;
    virtual Node2D& atNode(int pos);
    virtual double rangeMin(Axis which) const;
    virtual double rangeMax(Axis which) const;

    virtual FieldTri2D& define(int nodeId, double f);
    virtual double value(double x, double y, double outside=NAN) const;

private:
    std::vector<double> _valuelist; // list of all values, in nodeId order
    Ptr<MeshTri2D> _meshPtr;        // mesh for all (x,y) points
    int _counter;                   // counter for generating node IDs
};

} // namespace Rappture

#endif
