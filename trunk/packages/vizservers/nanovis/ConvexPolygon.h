/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ConvexPolygon.h: convex polygon class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef _CONVEX_POLYGON_H_
#define _CONVEX_POLYGON_H_

#include <assert.h>
#include <vector>

#include "Vector4.h"
#include "Plane.h"

typedef std::vector<Vector4> VertexVector;
typedef std::vector<Vector4> TexVector;

class ConvexPolygon
{
public:
    ConvexPolygon()
    {}

    ConvexPolygon(VertexVector vertices);

    void transform(const Mat4x4& mat);

    void translate(const Vector4& shift);

    // Clips the polygon, retaining the portion where ax + by + cz + d >= 0
    bool clip(Plane& clipPlane, bool copyToTexcoords);

    void emit(bool useTexture);

    void emit(bool useTexture, const Vector3& shift, const Vector3& scale);

    void copyVerticesToTexcoords();

    void setId(int id)
    { 
        volumeId = id; 
    }

    void appendVertex(const Vector4& vert)
    {
        vertices.push_back(vert);
    }

    void insertVertex(unsigned int index, const Vector4& vert)
    {
        assert(index < vertices.size());
        vertices.insert(vertices.begin() + index, vert);
    }

    VertexVector vertices;
    TexVector texcoords;
    int volumeId;	//which volume this polygon slice belongs to
};

#endif
