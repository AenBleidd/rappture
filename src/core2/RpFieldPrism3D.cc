/*
 * ----------------------------------------------------------------------
 *  Rappture::FieldPrism3D
 *    This is a continuous, linear function defined by a series of
 *    points on a 3D prismatic mesh.  It's a scalar field defined
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
#include "RpFieldPrism3D.h"

using namespace Rappture;

FieldPrism3D::FieldPrism3D()
  : _valuelist(),
    _meshPtr(NULL),
    _counter(0)
{
}

FieldPrism3D::FieldPrism3D(const MeshTri2D& xyg, const Mesh1D& zg)
  : _valuelist(),
    _meshPtr(NULL),
    _counter(0)
{
    _meshPtr = Ptr<MeshPrism3D>( new MeshPrism3D(xyg,zg) );
    int npts = xyg.sizeNodes()*zg.size();
    _valuelist.reserve(npts);
}

FieldPrism3D::FieldPrism3D(const FieldPrism3D& field)
  : _valuelist(field._valuelist),
    _meshPtr(field._meshPtr),
    _counter(field._counter)
{
}

FieldPrism3D&
FieldPrism3D::operator=(const FieldPrism3D& field)
{
    _valuelist = field._valuelist;
    _meshPtr = field._meshPtr;
    _counter = field._counter;
    return *this;
}

FieldPrism3D::~FieldPrism3D()
{
}

double
FieldPrism3D::rangeMin(Axis which) const
{
    if (!_meshPtr.isNull()) {
        return _meshPtr->rangeMin(which);
    }
    return 0.0;
}

double
FieldPrism3D::rangeMax(Axis which) const
{
    if (!_meshPtr.isNull()) {
        return _meshPtr->rangeMax(which);
    }
    return 0.0;
}

FieldPrism3D&
FieldPrism3D::define(int nodeId, double f)
{
    if (_valuelist.size() < (unsigned int)(nodeId+1)) {
        _valuelist.resize((unsigned int)(nodeId+1), NAN);
    }
    _valuelist[nodeId] = f;

    if (isnan(_vmin) || isnan(_vmax)) {
        _vmin = _vmax = f;
    } else {
        if (f < _vmin) { _vmin = f; }
        if (f > _vmax) { _vmax = f; }
    }
    return *this;
}

double
FieldPrism3D::value(double x, double y, double z, double outside) const
{
    if (!_meshPtr.isNull()) {
        CellPrism3D cell = _meshPtr->locate(Node3D(x,y,z));

        // outside the defined data? then return the outside value
        if (cell.isOutside()) {
            return outside;
        }

        // interpolate first xy triangle
        double fz0, fz1, phi[3];

        Node2D node( x, y );
        Node2D n1( cell.x(0), cell.y(0) );
        Node2D n2( cell.x(1), cell.y(1) );
        Node2D n3( cell.x(2), cell.y(2) );

        CellTri2D tri(0, &n1, &n2, &n3);
        tri.barycentrics(node, phi);

        fz0 = phi[0]*_valuelist[ cell.nodeId(0) ]
            + phi[1]*_valuelist[ cell.nodeId(1) ]
            + phi[2]*_valuelist[ cell.nodeId(2) ];

        // interpolate second xy triangle
        fz1 = phi[0]*_valuelist[ cell.nodeId(3) ]
            + phi[1]*_valuelist[ cell.nodeId(4) ]
            + phi[2]*_valuelist[ cell.nodeId(5) ];

        double zrange = cell.z(5) - cell.z(0);

        if (zrange == 0.0) {
            // interval undefined? then return avg value
            return 0.5*(fz1+fz0);
        }
        // interpolate along z-axis
        double delz = z - cell.z(0);
        return fz0 + (delz/zrange)*(fz1-fz0);
    }
    return outside;
}

double
FieldPrism3D::valueMin() const
{
    return _vmin;
}

double
FieldPrism3D::valueMax() const
{
    return _vmax;
}
