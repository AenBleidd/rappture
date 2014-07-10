/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Authors:
 *   Wei Qiao <qiaow@purdue.edu>
 */
#ifndef NV_CONVEX_POLYGON_H
#define NV_CONVEX_POLYGON_H

#include <assert.h>
#include <vector>

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>
#include <vrmath/Matrix4x4d.h>

#include "Plane.h"

namespace nv {

typedef std::vector<vrmath::Vector4f> VertexVector;
typedef std::vector<vrmath::Vector4f> TexVector;

class ConvexPolygon
{
public:
    ConvexPolygon()
    {}

    ConvexPolygon(VertexVector vertices);

    void transform(const vrmath::Matrix4x4d& mat);

    void translate(const vrmath::Vector4f& shift);

    // Clips the polygon, retaining the portion where ax + by + cz + d >= 0
    bool clip(nv::Plane& clipPlane, bool copyToTexcoords);

    void emit(bool useTexture);

    void emit(bool useTexture, const vrmath::Vector3f& shift, const vrmath::Vector3f& scale);

    void copyVerticesToTexcoords();

    void setId(int id)
    { 
        volumeId = id; 
    }

    void appendVertex(const vrmath::Vector4f& vert)
    {
        vertices.push_back(vert);
    }

    void insertVertex(unsigned int index, const vrmath::Vector4f& vert)
    {
        assert(index < vertices.size());
        vertices.insert(vertices.begin() + index, vert);
    }

    VertexVector vertices;
    TexVector texcoords;
    int volumeId;	//which volume this polygon slice belongs to
};

}

#endif