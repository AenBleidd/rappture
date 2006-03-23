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
#include <GL/glut.h>

Sphere::Sphere(float x, float y, float z, 
		float r, float g, float b, float a, 
		float _radius,
		int _stack,
		int _slice):
	center(Vector3(x,y,z)),
	color(Color(r,g,b,a)),
	radius(_radius),
	stack(_stack),
	slice(_slice)
{ }


Sphere::draw(GLUquadric* quad){
  glColor4f(color.r, color.g, color.b, color.a);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslate(center.x, center.y, center.z);

  //draw it
  gluSphere(quad, radius, stack, slice);
  glPopMatrix();
}

Sphere::set_horizontal_res(int _slice):slice(_slice){}
Sphere::set_vertical_res(int _stack):stack(_stack){}

