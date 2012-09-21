/*
 * ----------------------------------------------------------------------
 *  Rappture::MeshRect3D
 *    This is a non-uniform, structured 3D mesh.  Each of the x, y,
 *    and z-axes can be configured independently.  Their product
 *    forms a rectangular mesh in 3D space.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpMeshRect3D.h"

using namespace Rappture;

CellRect3D::CellRect3D()
{
    for (int i=0; i < 8; i++) {
        _nodeIds[i] = -1;
        _x[i] = 0.0;
        _y[i] = 0.0;
        _z[i] = 0.0;
    }
}

CellRect3D::CellRect3D(const CellRect3D& cell)
{
    for (int i=0; i < 8; i++) {
        _nodeIds[i] = cell._nodeIds[i];
        _x[i] = cell._x[i];
        _y[i] = cell._y[i];
        _z[i] = cell._z[i];
    }
}

CellRect3D&
CellRect3D::operator=(const CellRect3D& cell)
{
    for (int i=0; i < 8; i++) {
        _nodeIds[i] = cell._nodeIds[i];
        _x[i] = cell._x[i];
        _y[i] = cell._y[i];
        _z[i] = cell._z[i];
    }
    return *this;
}

int&
CellRect3D::nodeId(int n)
{
    assert(n >= 0 && n < 8);
    return _nodeIds[n];
}

double&
CellRect3D::x(int n)
{
    assert(n >= 0 && n < 8);
    return _x[n];
}

double&
CellRect3D::y(int n)
{
    assert(n >= 0 && n < 8);
    return _y[n];
}

double&
CellRect3D::z(int n)
{
    assert(n >= 0 && n < 8);
    return _z[n];
}

int
CellRect3D::isOutside() const
{
    for (int i=0; i < 8; i++) {
        if (_nodeIds[i] < 0) {
            return 1;
        }
    }
    return 0;
}


MeshRect3D::MeshRect3D()
{
}

MeshRect3D::MeshRect3D(const Mesh1D& xm, const Mesh1D& ym, const Mesh1D& zm)
{
    _axis[0] = xm;
    _axis[1] = ym;
    _axis[2] = zm;
}

MeshRect3D::MeshRect3D(const MeshRect3D& mesh)
    : _axis(mesh._axis)
{
}

MeshRect3D&
MeshRect3D::operator=(const MeshRect3D& mesh)
{
    _axis[0] = mesh._axis[0];
    _axis[1] = mesh._axis[1];
    _axis[2] = mesh._axis[2];
    return *this;
}

MeshRect3D::~MeshRect3D()
{
}

Node1D&
MeshRect3D::add(Axis which, const Node1D& node)
{
    return _axis[which].add(node);
}

MeshRect3D&
MeshRect3D::remove(Axis which, int nodeId)
{
    _axis[which].remove(nodeId);
    return *this;
}

MeshRect3D&
MeshRect3D::remove(Axis which, const Node1D& node)
{
    _axis[which].remove(node);
    return *this;
}

MeshRect3D&
MeshRect3D::clear()
{
    _axis[0].clear();
    _axis[1].clear();
    _axis[2].clear();
    return *this;
}

int
MeshRect3D::size(Axis which) const
{
    return _axis[which].size();
}

Node1D&
MeshRect3D::at(Axis which, int pos)
{
    return _axis[which].at(pos);
}

double
MeshRect3D::rangeMin(Axis which) const
{
    return _axis[which].rangeMin();
}

double
MeshRect3D::rangeMax(Axis which) const
{
    return _axis[which].rangeMax();
}

CellRect3D
MeshRect3D::locate(const Node3D& node) const
{
    CellRect3D result;
    Cell1D cells[3];
    Node1D coord(0.0);

    coord.x(node.x());
    cells[0] = _axis[0].locate(coord);

    coord.x(node.y());
    cells[1] = _axis[1].locate(coord);

    coord.x(node.z());
    cells[2] = _axis[2].locate(coord);

    int n = 0;
    for (int iz=0; iz < 2; iz++) {
        for (int iy=0; iy < 2; iy++) {
            for (int ix=0; ix < 2; ix++) {
                int nx = cells[0].nodeId(ix);
                int ny = cells[1].nodeId(iy);
                int nz = cells[2].nodeId(iz);
                if (nx >= 0 && ny >= 0 && nz >= 0) {
                    result.nodeId(n) = nz*(_axis[0].size()*_axis[1].size())
                                       + ny*(_axis[0].size())
                                       + nx;
                    result.x(n) = cells[0].x(ix);
                    result.y(n) = cells[1].x(iy);
                    result.z(n) = cells[2].x(iz);
                }
                n++;
            }
        }
    }
    return result;
}
