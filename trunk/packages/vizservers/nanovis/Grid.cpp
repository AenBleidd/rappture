/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include <stdlib.h>
#include <stdio.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include <util/Fonts.h>
#include <vrmath/Color4f.h>

#include "Grid.h"
#include "Trace.h"

using namespace nv;
using namespace nv::util;

#define NUMDIGITS	6
#define TICK_LENGTH	0.05
#define LABEL_OFFSET	0.06
#define TITLE_OFFSET	0.2

Grid::Grid() :
    xAxis("X"), 
    yAxis("Y"), 
    zAxis("Z"),
    _axisColor(1.0f, 1.0f, 1.0f, 1.0f), 
    _majorColor(1.0f, 1.0f, 1.0f, 1.0f), 
    _minorColor(0.5f, 0.5f, 0.5f, 1.0f), 
    _font(NULL), 
    _visible(false)
{
}

Grid::~Grid()
{
}

void Grid::getBounds(vrmath::Vector3f& bboxMin, vrmath::Vector3f& bboxMax) const
{
    bboxMin.set(xAxis.min(), yAxis.min(), zAxis.min());
    bboxMax.set(xAxis.min() + xAxis.range(),
                yAxis.min() + yAxis.range(),
                zAxis.min() + zAxis.range());
}

void Grid::render()
{
    if (!isVisible())
        return;

    glPushAttrib(GL_ENABLE_BIT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
#ifdef notdef
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
#else
    glDisable(GL_BLEND);
#endif

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glTranslatef(xAxis.min(), yAxis.min(), zAxis.min());
    glScalef(xAxis.range(), yAxis.range(), zAxis.range());

    glLineWidth(2.0f);
    glColor4f(_axisColor.r, _axisColor.g, _axisColor.b, 
              _axisColor.a);

    glBegin(GL_LINES);
    {
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(1.0f + TICK_LENGTH/xAxis.range(), 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 1.0f + TICK_LENGTH/yAxis.range(), 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 1.0f + TICK_LENGTH/zAxis.range());
    }
    glEnd();

    glLineWidth(1.0f);
    glColor4f(_majorColor.r, _majorColor.g, _majorColor.b, 
              _majorColor.a);

    glBegin(GL_LINES);
    {
        bool result;
        TickIter iter;

        for (result = xAxis.firstMajor(iter); result; result = iter.next()) {
            float x;
            x = xAxis.map(iter.getValue());
            glVertex3f(x, 0.0f, 0.0f);
            glVertex3f(x, 1.0f, 0.0f);
            glVertex3f(x, 0.0f, 0.0f);
            glVertex3f(x, 0.0f, 1.0f + TICK_LENGTH/zAxis.range());
        }
        for (result = yAxis.firstMajor(iter); result; result = iter.next()) {
            float y;
            y = yAxis.map(iter.getValue());
            glVertex3f(0.0f, y, 0.0f);
            glVertex3f(1.0f + TICK_LENGTH/xAxis.range(), y, 0.0f);
            glVertex3f(0.0f, y, 0.0f);
            glVertex3f(0.0f, y, 1.0f);
        }
        for (result = zAxis.firstMajor(iter); result; result = iter.next()) {
            float z;
            z = zAxis.map(iter.getValue());
            glVertex3f(0.0f, 0.0f, z);
            glVertex3f(0.0f, 1.0f, z);
            glVertex3f(0.0f, 0.0f, z);
            glVertex3f(1.0f + TICK_LENGTH/xAxis.range(), 0.0f, z);
        }
    }
    glEnd();

    // Set minor line color
    glColor4f(_minorColor.r, _minorColor.g, _minorColor.b,
              _minorColor.a);

    glBegin(GL_LINES);
    {
        bool result;
        TickIter iter;

        for (result = xAxis.firstMinor(iter); result; result = iter.next()) {
            float x;
            x = xAxis.map(iter.getValue());
            glVertex3f(x, 0.0f, 0.0f);
            glVertex3f(x, 1.0f, 0.0f);
            glVertex3f(x, 0.0f, 0.0f);
            glVertex3f(x, 0.0f, 1.0f);
        }
        for (result = yAxis.firstMinor(iter); result; result = iter.next()) {
            float y;
            y = yAxis.map(iter.getValue());
            glVertex3f(0.0f, y, 0.0f);
            glVertex3f(1.0f, y, 0.0f);
            glVertex3f(0.0f, y, 0.0f);
            glVertex3f(0.0f, y, 1.0f);
        }
        for (result = zAxis.firstMinor(iter); result; result = iter.next()) {
            float z;
            z = zAxis.map(iter.getValue());
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
        if (gluProject(1.0 + TITLE_OFFSET/xAxis.range(), 0.0, 0.0, mv, prjm, viewport, &wx, &wy, &wz) &&
            wz >= 0.0 && wz <= 1.0) {
            glLoadIdentity();
            glTranslatef((int) wx, viewport[3] - (int) wy, 0);
            const char *name = xAxis.name();
            if (name == NULL) {
                name = "???";
            }
            _font->draw(name);
        }
        
        if (gluProject(0.0, 1.0 + TITLE_OFFSET/yAxis.range(), 0.0, mv, prjm, viewport, &wx, &wy, &wz) &&
            wz >= 0.0 && wz <= 1.0) {
            glLoadIdentity();
            glTranslatef((int) wx, viewport[3] - (int)wy, 0);
            const char *name = yAxis.name();
            if (name == NULL) {
                name = "???";
            }
            _font->draw(name);
        }
        
        if (gluProject(0.0, 0.0, 1.0 + TITLE_OFFSET/zAxis.range(), mv, prjm, viewport, &wx, &wy, &wz) &&
            wz >= 0.0 && wz <= 1.0) {
            glLoadIdentity();
            glTranslatef((int) wx, (int) viewport[3] - (int)wy, 0.0f);
            const char *name = zAxis.name();
            if (name == NULL) {
                name = "???";
            }
            _font->draw(name);
        }
        
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f); 

        for (result = xAxis.firstMajor(iter); result; result = iter.next()) {
            float x;
            x = xAxis.map(iter.getValue());
            if (gluProject(x, 0.0f, 1.0 + LABEL_OFFSET/zAxis.range(), mv, prjm, viewport, &wx, &wy, &wz) &&
                wz >= 0.0 && wz <= 1.0) {
                char buff[20];
                glLoadIdentity();
                glTranslatef((int) wx, (int) viewport[3] - (int)wy, 0.0f);
                sprintf(buff, "%.*g", NUMDIGITS, iter.getValue());
                _font->draw(buff);
            }
        }
        for (result = yAxis.firstMajor(iter); result; result = iter.next()) {
            float y;
            y = yAxis.map(iter.getValue());
            if (gluProject(1.0 + LABEL_OFFSET/xAxis.range(), y, 0.0f, mv, prjm, viewport, &wx, &wy, &wz) &&
                wz >= 0.0 && wz <= 1.0) {
                char buff[20];
                glLoadIdentity();
                glTranslatef((int) wx, (int) viewport[3] - (int)wy, 0.0f);
                sprintf(buff, "%.*g", NUMDIGITS, iter.getValue());
                _font->draw(buff);
            }
        }
        for (result = zAxis.firstMajor(iter) + LABEL_OFFSET/xAxis.range(); result; result = iter.next()) {
            float z;
            z = zAxis.map(iter.getValue());
            if (gluProject(1.0 + LABEL_OFFSET/xAxis.range(), 0.0f, z, mv, prjm, viewport, &wx, &wy, &wz) &&
                wz >= 0.0 && wz <= 1.0) {
                char buff[20];
                glLoadIdentity();
                glTranslatef((int) wx, (int) viewport[3] - (int)wy, 0.0f);
                sprintf(buff, "%.*g", NUMDIGITS, iter.getValue());
                _font->draw(buff);
            }
        }
        _font->end();
    };

    glPopMatrix();
    glPopAttrib();
}

void Grid::setFont(Fonts *font)
{
    _font = font;
}

