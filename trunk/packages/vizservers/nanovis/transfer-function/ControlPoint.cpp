/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ControlPoint.h
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

#include <math.h>
#include "TransferFunctionMain.h"
#include "ControlPoint.h"


ControlPoint::ControlPoint(double _x, double _y)
{
	x = _x;
	y = _y;

	selected = false;
	dragged = false;
	
	next = 0;
}

ControlPoint::~ControlPoint()
{

}

void ControlPoint::glDraw()
{
	//Here is the code for drawing a vertex
	double size = 4;

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glTranslatef( x, y, 0 );

	glBegin(GL_LINES);

	glVertex2d(-size, -size);
	glVertex2d(+size, +size);

	glVertex2d(-size, +size);
	glVertex2d(+size, -size);
	
	glEnd();

	//Special code for drawing selected vertex and dragged vertex.

	if (selected && !dragged)
	{
		double square = size + 4;
		
		glBegin(GL_LINE_LOOP);
			glColor3d(1,0,0);			
			glVertex2d(-square, -square);
			glVertex2d(-square, +square);
			glVertex2d(+square, +square);
			glVertex2d(+square, -square);
			glColor3d(0,0,0);
		glEnd();

	}
	else if (selected && dragged)
	{
		double square = 4*sqrt(size + 4);
		glBegin(GL_LINE_LOOP);
			glColor3d(1,0,0);
			glVertex2d(-square, 0);
			glVertex2d(0, +square);
			glVertex2d(square, 0);
			glVertex2d(0, -square);
			glColor3d(0,0,0);
		glEnd();

	}
	glPopMatrix();

}



void ControlPoint::glDraw_2()
{
	//Here is the code for drawing a vertex
	double size = 4;

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glTranslatef( x, y, 0 );

	glBegin(GL_POLYGON);
		glVertex2d(-size, 0);
		glVertex2d(-size, -size);
		glVertex2d(+size, -size);
		glVertex2d(+size, 0);	
	glEnd();

	//Special code for drawing selected vertex and dragged vertex.

	if (selected && !dragged)
	{
		double square = size + 4;
		
		glBegin(GL_LINE_LOOP);
			glColor3d(1,0,0);			
			glVertex2d(-square, -square);
			glVertex2d(-square, +square);
			glVertex2d(+square, +square);
			glVertex2d(+square, -square);
			glColor3d(0,0,0);
		glEnd();

	}
	else if (selected && dragged)
	{
		double square = 4*sqrt(size + 4);
		glBegin(GL_LINE_LOOP);
			glColor3d(1,0,0);
			glVertex2d(-square, 0);
			glVertex2d(0, +square);
			glVertex2d(square, 0);
			glVertex2d(0, -square);
			glColor3d(0,0,0);
		glEnd();

	}
	glPopMatrix();

}


void ControlPoint::glDraw_3()
{
	//Here is the code for drawing a vertex
	double size = 7;

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glTranslatef( x, y, 0 );

	glBegin(GL_LINES);

	glVertex2d(-2, 0);
	glVertex2d(-size, 0);

	glVertex2d(2, 0);
	glVertex2d(size, 0);


	glVertex2d(0, -2);
	glVertex2d(0, -size);

	glVertex2d(0, 2);
	glVertex2d(0, size);

	glEnd();


	glPopMatrix();

}



void ControlPoint::Set(double x, double y)
{
	this->x = x;
	this->y = y;
}
