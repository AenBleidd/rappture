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

    double xLen = xAxis.range();
    double yLen = yAxis.range();
    double zLen = zAxis.range();
    double avgLen = 0.;
    double denom = 0.;
    if (xLen > 0.0) {
        avgLen += xLen;
        denom += 1.0;
    }
    if (yLen > 0.0) {
        avgLen += yLen;
        denom += 1.0;
    }
    if (zLen > 0.0) {
        avgLen += zLen;
        denom += 1.0;
    }
    if (denom > 0.0) {
        avgLen /= denom;
    } else {
        avgLen = 1.0;
    }
    float xTickLen = (TICK_LENGTH * avgLen) / xLen;
    float yTickLen = (TICK_LENGTH * avgLen) / yLen;
    float zTickLen = (TICK_LENGTH * avgLen) / zLen;
    float xLabelOfs = (LABEL_OFFSET * avgLen) / xLen;
    //float yLabelOfs = (LABEL_OFFSET * avgLen) / yLen;
    float zLabelOfs = (LABEL_OFFSET * avgLen) / zLen;
    float xTitleOfs = (TITLE_OFFSET * avgLen) / xLen;
    float yTitleOfs = (TITLE_OFFSET * avgLen) / yLen;
    float zTitleOfs = (TITLE_OFFSET * avgLen) / zLen;

    glTranslated(xAxis.min(), yAxis.min(), zAxis.min());
    glScaled(xLen, yLen, zLen);

    glLineWidth(2.0f);
    glColor4f(_axisColor.r, _axisColor.g, _axisColor.b, 
              _axisColor.a);

    glBegin(GL_LINES);
    {
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(1.0f + xTickLen, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 1.0f + yTickLen, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 1.0f + zTickLen);
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
            glVertex3f(x, 0.0f, 1.0f + zTickLen);
        }
        for (result = yAxis.firstMajor(iter); result; result = iter.next()) {
            float y;
            y = yAxis.map(iter.getValue());
            glVertex3f(0.0f, y, 0.0f);
            glVertex3f(1.0f + xTickLen, y, 0.0f);
            glVertex3f(0.0f, y, 0.0f);
            glVertex3f(0.0f, y, 1.0f);
        }
        for (result = zAxis.firstMajor(iter); result; result = iter.next()) {
            float z;
            z = zAxis.map(iter.getValue());
            glVertex3f(0.0f, 0.0f, z);
            glVertex3f(0.0f, 1.0f, z);
            glVertex3f(0.0f, 0.0f, z);
            glVertex3f(1.0f + xTickLen, 0.0f, z);
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

        glColor4f(_axisColor.r, _axisColor.g, _axisColor.b, 
                  _axisColor.a);

        if (gluProject(1.0 + xTitleOfs, 0.0, 0.0, mv, prjm, viewport, &wx, &wy, &wz) &&
            wz >= 0.0 && wz <= 1.0) {
            glLoadIdentity();
            glTranslatef((int) wx, viewport[3] - (int) wy, 0);
            const char *name = xAxis.name();
            if (name == NULL) {
                name = "???";
            }
            _font->draw(name);
        }
        
        if (gluProject(0.0, 1.0 + yTitleOfs, 0.0, mv, prjm, viewport, &wx, &wy, &wz) &&
            wz >= 0.0 && wz <= 1.0) {
            glLoadIdentity();
            glTranslatef((int) wx, viewport[3] - (int)wy, 0);
            const char *name = yAxis.name();
            if (name == NULL) {
                name = "???";
            }
            _font->draw(name);
        }
        
        if (gluProject(0.0, 0.0, 1.0 + zTitleOfs, mv, prjm, viewport, &wx, &wy, &wz) &&
            wz >= 0.0 && wz <= 1.0) {
            glLoadIdentity();
            glTranslatef((int) wx, (int) viewport[3] - (int)wy, 0.0f);
            const char *name = zAxis.name();
            if (name == NULL) {
                name = "???";
            }
            _font->draw(name);
        }
        
        glColor4f(_majorColor.r, _majorColor.g, _majorColor.b, 
                  _majorColor.a);

        for (result = xAxis.firstMajor(iter); result; result = iter.next()) {
            float x;
            x = xAxis.map(iter.getValue());
            if (gluProject(x, 0.0f, 1.0 + zLabelOfs, mv, prjm, viewport, &wx, &wy, &wz) &&
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
            if (gluProject(1.0 + xLabelOfs, y, 0.0f, mv, prjm, viewport, &wx, &wy, &wz) &&
                wz >= 0.0 && wz <= 1.0) {
                char buff[20];
                glLoadIdentity();
                glTranslatef((int) wx, (int) viewport[3] - (int)wy, 0.0f);
                sprintf(buff, "%.*g", NUMDIGITS, iter.getValue());
                _font->draw(buff);
            }
        }
        for (result = zAxis.firstMajor(iter); result; result = iter.next()) {
            float z;
            z = zAxis.map(iter.getValue());
            if (gluProject(1.0 + xLabelOfs, 0.0f, z, mv, prjm, viewport, &wx, &wy, &wz) &&
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

