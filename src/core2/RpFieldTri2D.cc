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
#include "RpFieldTri2D.h"

using namespace Rappture;

FieldTri2D::FieldTri2D()
  : _valuelist(),
    _vmin(NAN),
    _vmax(NAN),
    _meshPtr(NULL),
    _counter(0)
{
}

FieldTri2D::FieldTri2D(const MeshTri2D& grid)
  : _valuelist(),
    _vmin(NAN),
    _vmax(NAN),
    _meshPtr(NULL),
    _counter(0)
{
    _meshPtr = Ptr<MeshTri2D>( new MeshTri2D(grid) );
    _valuelist.reserve(grid.sizeNodes());
}

FieldTri2D::FieldTri2D(const FieldTri2D& field)
  : _valuelist(field._valuelist),
    _vmin(field._vmin),
    _vmax(field._vmax),
    _meshPtr(field._meshPtr),
    _counter(field._counter)
{
}

FieldTri2D&
FieldTri2D::operator=(const FieldTri2D& field)
{
    _valuelist = field._valuelist;
    _vmin = field._vmin;
    _vmax = field._vmax;
    _meshPtr = field._meshPtr;
    _counter = field._counter;
    return *this;
}

FieldTri2D::~FieldTri2D()
{
}

int
FieldTri2D::size() const
{
    return _meshPtr->sizeNodes();
}

Node2D&
FieldTri2D::atNode(int pos)
{
    static Node2D null(0.0,0.0);

    if (!_meshPtr.isNull()) {
        return _meshPtr->atNode(pos);
    }
    return null;
}

double
FieldTri2D::rangeMin(Axis which) const
{
    if (!_meshPtr.isNull()) {
        return _meshPtr->rangeMin(which);
    }
    return 0.0;
}

double
FieldTri2D::rangeMax(Axis which) const
{
    if (!_meshPtr.isNull()) {
        return _meshPtr->rangeMax(which);
    }
    return 0.0;
}

FieldTri2D&
FieldTri2D::define(int nodeId, double f)
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
FieldTri2D::value(double x, double y, double outside) const
{
    if (!_meshPtr.isNull()) {
        Node2D node(x,y);
        CellTri2D cell = _meshPtr->locate(node);

        if (!cell.isNull()) {
            double phi[3];
            cell.barycentrics(node, phi);

            int n0 = cell.nodeId(0);
            int n1 = cell.nodeId(1);
            int n2 = cell.nodeId(2);

            return phi[0]*_valuelist[n0]
                 + phi[1]*_valuelist[n1]
                 + phi[2]*_valuelist[n2];
        }
    }
    return outside;
}

double
FieldTri2D::valueMin() const
{
    return _vmin;
}

double
FieldTri2D::valueMax() const
{
    return _vmax;
}
