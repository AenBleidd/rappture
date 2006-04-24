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
#include <stdio.h>

Sphere::Sphere(float x, float y, float z, 
		float r, float g, float b,
		float _radius,
		int _stack,
		int _slice):
        Renderable(Vector3(x, y, z)),
	radius(_radius),
	stack(_stack),
	slice(_slice),
	color(Color(r,g,b))
{ 
  boundary = BoundBox(x-r, y-r, z-r, x+r, y+r, z+r);
}

Sphere::~Sphere(){}

void Sphere::render(){}

void Sphere::draw(GLUquadric* quad){
  glColor3f(color.R, color.G, color.B);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(location.x, location.y, location.z);

  //draw it
  gluSphere(quad, radius, stack, slice);

  glPopMatrix();
}

void Sphere::set_horizontal_res(int _slice) {slice=_slice;}
void Sphere::set_vertical_res(int _stack) {stack=_stack;}

