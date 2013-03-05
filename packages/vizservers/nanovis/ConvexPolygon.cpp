 /* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ConvexPolygon.cpp: convex polygon class
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
#include <assert.h>

#include <GL/glew.h>

#include "ConvexPolygon.h"
#include "Trace.h"

ConvexPolygon::ConvexPolygon(VertexVector newvertices)
{
    vertices.insert(vertices.begin(), newvertices.begin(), newvertices.end());
}

void
ConvexPolygon::transform(const Mat4x4& mat)
{
    VertexVector tmp = vertices;
    vertices.clear();

    for (unsigned int i = 0; i < tmp.size(); i++) {
        Vector4 vec = tmp[i];
        vertices.push_back(mat.transform(vec));
    }
}

void
ConvexPolygon::translate(const Vector4& shift)
{
    VertexVector tmp = vertices;
    vertices.clear();

    for (unsigned int i = 0; i < tmp.size(); i++) {
        Vector4 vec = tmp[i];
        vertices.push_back(vec+shift);
    }
}

#define SIGN_DIFFERS(x, y) \
    ( ((x) < 0.0 && (y) > 0.0) || ((x) > 0.0 && (y) < 0.0) )

bool
ConvexPolygon::clip(Plane& clipPlane, bool copyToTexcoord)
{
    if (vertices.size() == 0) {
        ERROR("polygon has no vertices");
        return false;
    }

    VertexVector clippedVerts;
    clippedVerts.reserve(2 * vertices.size());

    // The algorithm is as follows: for each vertex
    // in the current poly, check to see which 
    // half space it is in: clipped (outside) or inside.
    // If the previous vertex was in the other
    // half space, find the intersection of that edge
    // with the clip plane and add that intersection
    // point to the new list of vertices.  If the 
    // current vertex is in the inside half-space,
    // add it to the new list as well.

    Vector4 plane = clipPlane.getCoeffs();

    // This implementation is based on the Mesa 3D library (MIT license)
    int v1 = 0;
    // dot product >= 0 on/inside half-space, < 0 outside half-space
    float dot1 = vertices[v1] * plane;
    for (unsigned int i = 1; i <= vertices.size(); i++) {
        int v2 = (i == vertices.size()) ? 0 : i;
        float dot2 = vertices[v2] * plane;
        if (dot1 >= 0.0) {
            // on/inside
            clippedVerts.push_back(vertices[v1]);
        }
        if (SIGN_DIFFERS(dot1, dot2)) {
            if (dot1 < 0.0) {
                // outside -> inside
                double t = dot1 / (dot1 - dot2);
                clippedVerts.push_back(vlerp(vertices[v1], vertices[v2], t));
            } else {
                // inside -> outside
                double t = dot2 / (dot2 - dot1);
                clippedVerts.push_back(vlerp(vertices[v2], vertices[v1], t));
            }
        }
        dot1 = dot2;
        v1 = v2;
    }

    vertices.clear();

    if (clippedVerts.size() < 3) {
        texcoords.clear();
        return false;
    } else {
        vertices.insert(vertices.begin(), 
                        clippedVerts.begin(),
                        clippedVerts.end());

        if (copyToTexcoord)
            copyVerticesToTexcoords();

        return true;
    }
}

#undef SIGN_DIFFERS

void
ConvexPolygon::copyVerticesToTexcoords()
{
    if (texcoords.size() > 0)
        texcoords.clear();

    for (unsigned int i = 0; i < vertices.size(); i++) {
        texcoords.push_back(vertices[i]);
    }
}

void
ConvexPolygon::emit(bool useTexture)
{
    if (vertices.size() >= 3) {
	for (unsigned int i = 0; i < vertices.size(); i++) {
	    if (useTexture) {
		glTexCoord4fv((float *)&(texcoords[i]));
		//glTexCoord4fv((float *)&(vertices[i]));
	    }
	    glVertex4fv((float *)&(vertices[i]));
	}
    } else {
        WARN("No polygons to render");
    }
}

void 
ConvexPolygon::emit(bool useTexture, const Vector3& shift, const Vector3& scale) 
{
    if (vertices.size() >= 3) {
	for (unsigned int i = 0; i < vertices.size(); i++) {
	    if (useTexture) {
		glTexCoord4fv((float *)&(vertices[i]));
	    }
	    Vector4 tmp = (vertices[i]);
	    Vector4 shift_4d = Vector4(shift.x, shift.y, shift.z, 0);
	    tmp = tmp + shift_4d;
	    tmp.x = tmp.x * scale.x;
	    tmp.y = tmp.y * scale.y;
	    tmp.z = tmp.z * scale.z;
	    glVertex4fv((float *)(&tmp));
	}
    } else {
        WARN("No polygons to render");
    }
}
