/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Sphere.h : Sphere class
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
#ifndef SPHERE_H
#define SPHERE_H

#include <GL/glew.h>
#include <GL/glu.h>

#include "Trace.h"

#include "Color.h"
#include "Renderable.h"

class Sphere : public Renderable
{
public:
    Sphere(float x, float y, float z,
           float r, float g, float b,
           float radius,
	   int stack, int slice);

    virtual ~Sphere();

    void setVerticalRes(int stack);
    void setHorizontalRes(int slice);

    //display the sphere
    void draw(GLUquadric *q);
    void render()
    {}

private:
    float _radius;
    Color _color;
    int _stack;
    int _slice;
};

#endif
