/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Sphere.h : Sphere class
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

#ifndef _SPHERE_H_
#define _SPHERE_H_

#include <GL/glew.h>
#include <GL/glu.h>

#include "Trace.h"

#include "Color.h"
#include "Renderable.h"

class Sphere : public Renderable
{
public:
    float radius;
    Color color;
    int stack;
    int slice;

    Sphere(float x, float y, float z, float r, float g, float b, float _radius,
	   int _stack, int _slice);

    virtual ~Sphere();

    void set_vertical_res(int _stack);
    void set_horizontal_res(int _slice);
        
    //display the sphere
    void draw(GLUquadric* q);
    void render();
};

#endif
