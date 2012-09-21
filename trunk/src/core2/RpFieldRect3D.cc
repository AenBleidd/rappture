/*
 * ----------------------------------------------------------------------
 *  Rappture::FieldRect3D
 *    This is a continuous, linear function defined by a series of
 *    points on a 3D structured mesh.  It's a scalar field defined
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
#include "RpFieldRect3D.h"

using namespace Rappture;

FieldRect3D::FieldRect3D()
  : _valuelist(),
    _vmin(NAN),
    _vmax(NAN),
    _meshPtr(NULL),
    _counter(0)
{
}

FieldRect3D::FieldRect3D(const Mesh1D& xg, const Mesh1D& yg, const Mesh1D& zg)
  : _valuelist(),
    _vmin(NAN),
    _vmax(NAN),
    _meshPtr(NULL),
    _counter(0)
{
    _meshPtr = Ptr<MeshRect3D>( new MeshRect3D(xg,yg,zg) );
    int npts = xg.size()*yg.size()*zg.size();
    _valuelist.reserve(npts);
}

FieldRect3D::FieldRect3D(const FieldRect3D& field)
  : _valuelist(field._valuelist),
    _vmin(field._vmin),
    _vmax(field._vmax),
    _meshPtr(field._meshPtr),
    _counter(field._counter)
{
}

FieldRect3D&
FieldRect3D::operator=(const FieldRect3D& field)
{
    _valuelist = field._valuelist;
    _vmin = field._vmin;
    _vmax = field._vmax;
    _meshPtr = field._meshPtr;
    _counter = field._counter;
    return *this;
}

FieldRect3D::~FieldRect3D()
{
}

int
FieldRect3D::size(Axis which) const
{
    if (!_meshPtr.isNull()) {
        return _meshPtr->size(which);
    }
    return 0;
}

Node1D&
FieldRect3D::atNode(Axis which, int pos)
{
    static Node1D null(0.0);

    if (!_meshPtr.isNull()) {
        return _meshPtr->at(which, pos);
    }
    return null;
}

double
FieldRect3D::rangeMin(Axis which) const
{
    if (!_meshPtr.isNull()) {
        return _meshPtr->rangeMin(which);
    }
    return 0.0;
}

double
FieldRect3D::rangeMax(Axis which) const
{
    if (!_meshPtr.isNull()) {
        return _meshPtr->rangeMax(which);
    }
    return 0.0;
}

FieldRect3D&
FieldRect3D::define(int nodeId, double f)
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
FieldRect3D::value(double x, double y, double z, double outside) const
{
    double f0, f1, fy0, fy1, fz0, fz1;

    if (!_meshPtr.isNull()) {
        CellRect3D cell = _meshPtr->locate(Node3D(x,y,z));

        // outside the defined data? then return the outside value
        if (cell.isOutside()) {
            return outside;
        }

        // yuck! brute force...
        // interpolate x @ y0,z0
        f0 = _valuelist[cell.nodeId(0)];
        f1 = _valuelist[cell.nodeId(1)];
        fy0 = _interpolate(cell.x(0),f0, cell.x(1),f1, x);

        // interpolate x @ y1,z0
        f0 = _valuelist[cell.nodeId(2)];
        f1 = _valuelist[cell.nodeId(3)];
        fy1 = _interpolate(cell.x(2),f0, cell.x(3),f1, x);

        // interpolate y @ z0
        fz0 = _interpolate(cell.y(0),fy0, cell.y(2),fy1, y);

        // interpolate x @ y0,z1
        f0 = _valuelist[cell.nodeId(4)];
        f1 = _valuelist[cell.nodeId(5)];
        fy0 = _interpolate(cell.x(4),f0, cell.x(5),f1, x);

        // interpolate x @ y1,z1
        f0 = _valuelist[cell.nodeId(6)];
        f1 = _valuelist[cell.nodeId(7)];
        fy1 = _interpolate(cell.x(6),f0, cell.x(7),f1, x);

        // interpolate y @ z1
        fz1 = _interpolate(cell.y(4),fy0, cell.y(6),fy1, y);

        // interpolate z
        return _interpolate(cell.z(0),fz0, cell.z(4),fz1, z);
    }
    return outside;
}

double
FieldRect3D::valueMin() const
{
    return _vmin;
}

double
FieldRect3D::valueMax() const
{
    return _vmax;
}

double
FieldRect3D::_interpolate(double x0, double y0, double x1, double y1,
    double x) const
{
    double dx = x1-x0;
    if (dx == 0.0) {
        return 0.5*(y0+y1);
    }
    return (y1-y0)*((x-x0)/dx) + y0;
}

