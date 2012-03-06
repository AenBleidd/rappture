/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * texture3d.h: 3d texture class
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
#ifndef _TEXTURE_3D_H_
#define _TEXTURE_3D_H_

#include <GL/glew.h>
#include "config.h"

class Texture3D{
	

public:
	int width;
	int height;
	int depth;

	double aspect_ratio_width;
	double aspect_ratio_height;
	double aspect_ratio_depth;

	GLuint type;
	GLuint interp_type;
	int n_components;
	bool gl_resource_allocated;

	GLuint id;
	GLuint tex_unit;

	Texture3D();
	Texture3D(int width, int height, int depth, GLuint type, GLuint interp, int n);
	~Texture3D();
	
	void activate();
	void deactivate();
	GLuint initialize(float* data);
	static void check_max_size();
	static void check_max_unit();

    void update(float* data);

};

#endif
