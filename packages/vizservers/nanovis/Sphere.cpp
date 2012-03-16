/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Sphere.cpp : Sphere class
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

#include "Sphere.h"

Sphere::Sphere(float x, float y, float z, 
               float r, float g, float b,
               float radius,
               int stack,
               int slice) :
    Renderable(Vector3(x, y, z)),
    _radius(radius),
    _color(Color(r, g, b)),
    _stack(stack),
    _slice(slice)
{ 
}

Sphere::~Sphere()
{
}

void
Sphere::draw(GLUquadric *quad)
{
    glColor3f(_color.r(), _color.g(), _color.b());

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(_location.x, _location.y, _location.z);

    //draw it
    gluSphere(quad, _radius, _stack, _slice);

    glPopMatrix();
}

void
Sphere::set_horizontal_res(int slice)
{
    _slice = slice;
}

void
Sphere::set_vertical_res(int stack)
{
    _stack = stack;
}
