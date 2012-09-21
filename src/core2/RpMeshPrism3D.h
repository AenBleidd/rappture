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
#ifndef RPMESHPRISM3D_H
#define RPMESHPRISM3D_H

#include <RpMesh1D.h>
#include <RpMeshTri2D.h>

namespace Rappture {

class CellPrism3D {
public:
    CellPrism3D();
    CellPrism3D(const CellPrism3D& cell);
    CellPrism3D& operator=(const CellPrism3D& cell);
    int& nodeId(int n);
    double& x(int n);
    double& y(int n);
    double& z(int n);

    int isOutside() const;

private:
    int _nodeIds[6];
    double _x[6];
    double _y[6];
    double _z[6];
};

class MeshPrism3D {
public:
    MeshPrism3D();
    MeshPrism3D(const MeshTri2D& xym, const Mesh1D& zm);
    MeshPrism3D(const MeshPrism3D& mesh);
    MeshPrism3D& operator=(const MeshPrism3D& mesh);
    virtual ~MeshPrism3D();

    virtual double rangeMin(Axis which) const;
    virtual double rangeMax(Axis which) const;

    virtual CellPrism3D locate(const Node3D& node) const;

    // required for base class Serializable:
    const char* serializerType() const { return "MeshPrism3D"; }
    char serializerVersion() const { return 'A'; }

private:
    MeshTri2D _xymesh;  // triangular mesh in x/y axes
    Mesh1D _zmesh;      // mesh along z-axis
};

} // namespace Rappture

#endif
