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
#ifndef RAPPTURE_MESHRECT3D_H
#define RAPPTURE_MESHRECT3D_H

#include "RpMesh1D.h"

namespace Rappture {

class CellRect3D {
public:
    CellRect3D();
    CellRect3D(const CellRect3D& cell);
    CellRect3D& operator=(const CellRect3D& cell);
    int& nodeId(int n);
    double& x(int n);
    double& y(int n);
    double& z(int n);

    int isOutside() const;

private:
    int _nodeIds[8];
    double _x[8];
    double _y[8];
    double _z[8];
};

class MeshRect3D {
public:
    MeshRect3D();
    MeshRect3D(const Mesh1D& xm, const Mesh1D& ym, const Mesh1D& zm);
    MeshRect3D(const MeshRect3D& mesh);
    MeshRect3D& operator=(const MeshRect3D& mesh);
    virtual ~MeshRect3D();

    virtual Node1D& add(Axis which, const Node1D& node);
    virtual MeshRect3D& remove(Axis which, int nodeId);
    virtual MeshRect3D& remove(Axis which, const Node1D& node);
    virtual MeshRect3D& clear();

    virtual int size(Axis which) const;
    virtual Node1D& at(Axis which, int pos);
    virtual double rangeMin(Axis which) const;
    virtual double rangeMax(Axis which) const;

    virtual CellRect3D locate(const Node3D& node) const;

    // required for base class Serializable:
    const char* serializerType() const { return "MeshRect3D"; }
    char serializerVersion() const { return 'A'; }

private:
    Mesh1D _axis[3];  // mesh along x-, y-, and z-axes
};

} // namespace Rappture

#endif
