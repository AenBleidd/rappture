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

// Finds the intersection of the line through 
// p1 and p2 with the given plane, putting the
// result in intersect.
//
// If the line lies in the plane, an arbitrary 
// point on the line is returned.
//
// http://astronomy.swin.edu.au/pbourke/geometry/planeline/
bool 
findIntersection(const Vector4& pt1, const Vector4& pt2, const Vector4& plane, Vector4& ret)
{
    float a = plane.x;
    float b = plane.y;
    float c = plane.z;
    float d = plane.w;

    Vector4 p1 = pt1;
    p1.perspectiveDivide();
    float x1 = p1.x;
    float y1 = p1.y;
    float z1 = p1.z;

    Vector4 p2 = pt2;
    p2.perspectiveDivide();
    float x2 = p2.x;
    float y2 = p2.y;
    float z2 = p2.z;

    float uDenom = a * (x1 - x2) + b * (y1 - y2) + c * (z1 - z2);
    float uNumer = a * x1 + b * y1 + c * z1 + d;

    if (uDenom == 0){ 
        //plane parallel to line
        ERROR("Unexpected code path: ConvexPolygon.cpp\n");
        if (uNumer == 0){
            ret = p1;
            return true;
        }
        return false;
    }

    float u = uNumer / uDenom;
    ret.x = x1 + u * (x2 - x1);
    ret.y = y1 + u * (y2 - y1);
    ret.z = z1 + u * (z2 - z1);
    ret.w = 1;
    return true;
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

void
ConvexPolygon::clip(Plane& clipPlane, bool copyToTexcoord)
{
    if (vertices.size() == 0) {
        //ERROR("ConvexPolygon: polygon has no vertices\n");  
        return;
    }
    
    VertexVector clippedVerts;
    clippedVerts.reserve(2 * vertices.size());

    // The algorithm is as follows: for each vertex
    // in the current poly, check to see which 
    // half space it is in: clipped or retained.
    // If the previous vertex was in the other
    // half space, find the intersect of that edge
    // with the clip plane and add that intersect
    // point to the new list of vertices.  If the 
    // current vertex is in the retained half-space,
    // add it to the new list as well.

    Vector4 intersect;
    Vector4 plane = clipPlane.getCoeffs();

    bool prevRetained = isRetained(vertices[0], plane);
    if (prevRetained) 
        clippedVerts.push_back(vertices[0]);

    for (unsigned int i = 1; i < vertices.size(); i++) {
        bool retained = isRetained(vertices[i], plane);
        if (retained != prevRetained) {
            bool found = findIntersection(vertices[i - 1], vertices[i], 
                                          plane, intersect);
            assert(found);
            clippedVerts.push_back(intersect);
        }
        if (retained) {
            clippedVerts.push_back(vertices[i]);
        }
        prevRetained = retained;
    }

    bool retained = isRetained(vertices[0], plane);
    if (retained != prevRetained) {
        bool found = findIntersection(vertices[vertices.size() - 1], 
				      vertices[0], plane, intersect);
        assert(found);
        clippedVerts.push_back(intersect);
    }

    vertices.clear();
    vertices.insert(vertices.begin(), 
                    clippedVerts.begin(),
                    clippedVerts.end());

    if (copyToTexcoord)
        copyVerticesToTexcoords();
}

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
    }
}
