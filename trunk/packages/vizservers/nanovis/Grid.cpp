/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include "Grid.h"

#define NUMDIGITS	6
#define GRID_TICK	0.05

Grid::Grid() : 
    _axisColor(1.0f, 1.0f, 1.0f, 1.0f), 
    _majorColor(1.0f, 1.0f, 1.0f, 1.0f), 
    _minorColor(0.5f, 0.5f, 0.5f, 1.0f), 
    _font(0), 
    _visible(false),
    xAxis("X"), 
    yAxis("Y"), 
    zAxis("Z")
{
    /*empty*/
}

void Grid::render()
{
    glPushMatrix();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

#ifdef notdef
    glEnable(GL_LINE_SMOOTH);
#endif
#ifdef notdef
    glScalef(xAxis.scale(), 
	     yAxis.range() / xAxis.range(), 
	     zAxis.range() / xAxis.range());
#endif
    glScalef(1.0, 1.0, 1.0);

    glTranslatef(-0.5f, -0.5f, -0.5f);
    glLineWidth(2.0f);
    glColor4f(_axisColor.red, _axisColor.green, _axisColor.blue, 
	      _axisColor.alpha);

    glBegin(GL_LINES);
    {
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(1.0f + GRID_TICK, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f + GRID_TICK, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 1.0f + GRID_TICK);
    }
    glEnd();

    glLineWidth(1.0f);
    glColor4f(_majorColor.red, _majorColor.green, _majorColor.blue, 
	      _majorColor.alpha);

    glBegin(GL_LINES);
    {
	bool result;
	TickIter iter;

	for (result = xAxis.FirstMajor(iter); result; result = iter.Next()) {
	    float x;
	    x = xAxis.Map(iter.GetValue());
	    glVertex3f(x, 0.0f, 0.0f);
	    glVertex3f(x, 1.0f, 0.0f);
	    glVertex3f(x, 0.0f, 0.0f);
	    glVertex3f(x, 0.0f, 1.0f + GRID_TICK);
	}
	for (result = yAxis.FirstMajor(iter); result; result = iter.Next()) {
	    float y;
	    y = yAxis.Map(iter.GetValue());
	    glVertex3f(0.0f, y, 0.0f);
	    glVertex3f(1.0f + GRID_TICK, y, 0.0f);
	    glVertex3f(0.0f, y, 0.0f);
	    glVertex3f(0.0f, y, 1.0f);
	}
	for (result = zAxis.FirstMajor(iter); result; result = iter.Next()) {
	    float z;
	    z = zAxis.Map(iter.GetValue());
	    glVertex3f(0.0f, 0.0f, z);
	    glVertex3f(0.0f, 1.0f, z);
	    glVertex3f(0.0f, 0.0f, z);
	    glVertex3f(1.0f + GRID_TICK, 0.0f, z);
	}
    }
    glEnd();

    // Set minor line color
    glColor4f(_minorColor.red, _minorColor.green, _minorColor.blue,
	      _minorColor.alpha);

    glBegin(GL_LINES);
    {
	bool result;
	TickIter iter;

	for (result = xAxis.FirstMinor(iter); result; result = iter.Next()) {
	    float x;
	    x = xAxis.Map(iter.GetValue());
	    glVertex3f(x, 0.0f, 0.0f);
	    glVertex3f(x, 1.0f, 0.0f);
	    glVertex3f(x, 0.0f, 0.0f);
	    glVertex3f(x, 0.0f, 1.0f);
	}
	for (result = yAxis.FirstMinor(iter); result; result = iter.Next()) {
	    float y;
	    y = yAxis.Map(iter.GetValue());
	    glVertex3f(0.0f, y, 0.0f);
	    glVertex3f(1.0f, y, 0.0f);
	    glVertex3f(0.0f, y, 0.0f);
	    glVertex3f(0.0f, y, 1.0f);
	}
	for (result = zAxis.FirstMinor(iter); result; result = iter.Next()) {
	    float z;
	    z = zAxis.Map(iter.GetValue());
	    glVertex3f(0.0f, 0.0f, z);
	    glVertex3f(0.0f, 1.0f, z);
	    glVertex3f(0.0f, 0.0f, z);
	    glVertex3f(1.0f, 0.0f, z);
	}
    }
    glEnd();

    if (_font != NULL) {
	double mv[16], prjm[16];
	int viewport[4];
	double wx, wy, wz;
	bool result;
	TickIter iter;

	glGetDoublev(GL_MODELVIEW_MATRIX, mv);
	glGetDoublev(GL_PROJECTION_MATRIX, prjm);
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	_font->begin();
	if (gluProject(1.2, 0.0, 0.0, mv, prjm, viewport, &wx, &wy, &wz)) {
	    glLoadIdentity();
	    glTranslatef((int) wx, viewport[3] - (int) wy, 0);
	    const char *name = xAxis.name();
	    if (name == NULL) {
		name = "???";
	    }
	    _font->draw(name);
	}
	
	if (gluProject(0.0, 1.2, 0.0, mv, prjm, viewport, &wx, &wy, &wz)) {
	    glLoadIdentity();
	    glTranslatef((int) wx, viewport[3] - (int)wy, 0);
	    const char *name = yAxis.name();
	    if (name == NULL) {
		name = "???";
	    }
	    _font->draw(name);
	}
	
	if (gluProject(0.0, 0.0, 1.2, mv, prjm, viewport, &wx, &wy, &wz)) {
	    glLoadIdentity();
	    glTranslatef((int) wx, (int) viewport[3] - (int)wy, 0.0f);
	    const char *name = zAxis.name();
	    if (name == NULL) {
		name = "???";
	    }
	    _font->draw(name);
	}
	
	glColor4f(1.0f, 1.0f, 0.0f, 1.0f); 

	for (result = xAxis.FirstMajor(iter); result; result = iter.Next()) {
	    float x;
	    x = xAxis.Map(iter.GetValue());
	    if (gluProject(x, 0.0f, 1.06f, mv, prjm, viewport, &wx, &wy, &wz)) {
		char buff[20];
		glLoadIdentity();
		glTranslatef((int) wx, (int) viewport[3] - (int)wy, 0.0f);
		sprintf(buff, "%.*g", NUMDIGITS, iter.GetValue());
		_font->draw(buff);
	    }
	}
	for (result = yAxis.FirstMajor(iter); result; result = iter.Next()) {
	    float y;
	    y = yAxis.Map(iter.GetValue());
	    if (gluProject(1.06f, y, 0.0f, mv, prjm, viewport, &wx, &wy, &wz)) {
		char buff[20];
		glLoadIdentity();
		glTranslatef((int) wx, (int) viewport[3] - (int)wy, 0.0f);
		sprintf(buff, "%.*g", NUMDIGITS, iter.GetValue());
		_font->draw(buff);
	    }
	}
	for (result = zAxis.FirstMajor(iter); result; result = iter.Next()) {
	    float z;
	    z = zAxis.Map(iter.GetValue());
	    if (gluProject(1.06f, 0.0f, z, mv, prjm, viewport, &wx, &wy, &wz)) {
		char buff[20];
		glLoadIdentity();
		glTranslatef((int) wx, (int) viewport[3] - (int)wy, 0.0f);
		sprintf(buff, "%.*g", NUMDIGITS, iter.GetValue());
		_font->draw(buff);
	    }
	}
	_font->end();
    };
    glPopMatrix();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
#ifdef notdef
    glDisable(GL_LINE_SMOOTH);
#endif
}
 
void Grid::setFont(R2Fonts* font)
{
//	if (font == _font) return;
//	if (font) font->addRef();
//	if (_font) _font->unrefDelete();
	
    _font = font;
}

