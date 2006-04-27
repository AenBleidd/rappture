/*
 * ----------------------------------------------------------------------
 * ConvexPolygon.h: convex polygon class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef _CONVEX_POLYGON_H_
#define _CONVEX_POLYGON_H_

#include "Vector4.h"
#include "Plane.h"
#include <vector>

typedef std::vector<Vector4> VertexVector;
typedef std::vector<Vector4> TexVector;

class ConvexPolygon {
public:
	VertexVector vertices;
	TexVector texcoords;
	int volume_id;	//which volume this polygon slice belongs to

	ConvexPolygon();
	ConvexPolygon(VertexVector vertices);
	
	void append_vertex(Vector4 vert);
	void insert_vertex(unsigned int index, Vector4 vert);
	void transform(Mat4x4 mat);
	void translate(Vector4 shift);

	// Clips the polygon, retaining the portion where ax + by + cz + d >= 0
	void clip(Plane &clipPlane, bool copy_to_texcoords);
	void Emit(bool use_texture);
	void Emit(bool use_texture, Vector3& shift, Vector3& scale);
	void copy_vertices_to_texcoords();
	void set_id(int v_id);
};

#endif
