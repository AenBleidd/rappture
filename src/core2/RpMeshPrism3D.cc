/*
 * ----------------------------------------------------------------------
 *  Rappture::MeshPrism3D
 *    This is a non-uniform 3D mesh, made of triangles in (x,y) and
 *    line segments in z.  Their product forms triangular prisms.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <assert.h>
#include "RpMeshPrism3D.h"

using namespace Rappture;

CellPrism3D::CellPrism3D()
{
    for (int i=0; i < 6; i++) {
        _nodeIds[i] = -1;
        _x[i] = 0.0;
        _y[i] = 0.0;
        _z[i] = 0.0;
    }
}

CellPrism3D::CellPrism3D(const CellPrism3D& cell)
{
    for (int i=0; i < 6; i++) {
        _nodeIds[i] = cell._nodeIds[i];
        _x[i] = cell._x[i];
        _y[i] = cell._y[i];
        _z[i] = cell._z[i];
    }
}

CellPrism3D&
CellPrism3D::operator=(const CellPrism3D& cell)
{
    for (int i=0; i < 6; i++) {
        _nodeIds[i] = cell._nodeIds[i];
        _x[i] = cell._x[i];
        _y[i] = cell._y[i];
        _z[i] = cell._z[i];
    }
    return *this;
}

int&
CellPrism3D::nodeId(int n)
{
    assert(n >= 0 && n < 6);
    return _nodeIds[n];
}

double&
CellPrism3D::x(int n)
{
    assert(n >= 0 && n < 6);
    return _x[n];
}

double&
CellPrism3D::y(int n)
{
    assert(n >= 0 && n < 6);
    return _y[n];
}

double&
CellPrism3D::z(int n)
{
    assert(n >= 0 && n < 6);
    return _z[n];
}

int
CellPrism3D::isOutside() const
{
    for (int i=0; i < 6; i++) {
        if (_nodeIds[i] < 0) {
            return 1;
        }
    }
    return 0;
}

MeshPrism3D::MeshPrism3D()
{
}

MeshPrism3D::MeshPrism3D(const MeshTri2D& xym, const Mesh1D& zm)
{
    _xymesh = xym;
    _zmesh  = zm;
}

MeshPrism3D::MeshPrism3D(const MeshPrism3D& mesh)
    : _xymesh(mesh._xymesh),
      _zmesh(mesh._zmesh)
{
}

MeshPrism3D&
MeshPrism3D::operator=(const MeshPrism3D& mesh)
{
    _xymesh = mesh._xymesh;
    _zmesh  = mesh._zmesh;
    return *this;
}

MeshPrism3D::~MeshPrism3D()
{
}

double
MeshPrism3D::rangeMin(Axis which) const
{
    if (which == zaxis) {
        return _zmesh.rangeMin();
    }
    return _xymesh.rangeMin(which);
}

double
MeshPrism3D::rangeMax(Axis which) const
{
    if (which == zaxis) {
        return _zmesh.rangeMax();
    }
    return _xymesh.rangeMax(which);
}

CellPrism3D
MeshPrism3D::locate(const Node3D& node) const
{
    CellPrism3D result;

    Node2D xycoord(node.x(), node.y());
    CellTri2D xycell = _xymesh.locate(xycoord);

    Node1D zcoord(node.z());
    Cell1D zcell = _zmesh.locate(zcoord);

    if (!xycell.isOutside() && !zcell.isOutside()) {
        int n = 0;
        for (int iz=0; iz < 2; iz++) {
            for (int ixy=0; ixy < 3; ixy++) {
                int nxy = xycell.nodeId(ixy);
                int nz = zcell.nodeId(iz);
                if (nxy >= 0 && nz >= 0) {
                    result.nodeId(n) = nz*_xymesh.sizeNodes() + nxy;
                    result.x(n) = xycell.x(ixy);
                    result.y(n) = xycell.y(ixy);
                    result.z(n) = zcell.x(iz);
                }
                n++;
            }
        }
    }
    return result;
}
