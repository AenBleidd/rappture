/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdlib.h>
#include <stdio.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include "Grid.h"
#include "Trace.h"

#define NUMDIGITS	6
#define GRID_TICK	0.05

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

void Grid::render()
{
    glPushAttrib(GL_ENABLE_BIT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
#ifdef notdef
    glEnable(GL_LINE_SMOOTH);
#endif

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    double xDataRange = xAxis.dataMax() - xAxis.dataMin();
    double yDataRange = yAxis.dataMax() - yAxis.dataMin();
    double zDataRange = zAxis.dataMax() - zAxis.dataMin();

    double paspectX = 1.0f;
    double paspectY = yDataRange / xDataRange;
    double paspectZ = zDataRange / xDataRange;
 
    double xscale = xAxis.range() / xDataRange;
    double yscale = yAxis.range() / xDataRange;
    double zscale = zAxis.range() / xDataRange;

    double xoffset = (xAxis.min() - xAxis.dataMin()) * xAxis.scale();
    double yoffset = (yAxis.min() - yAxis.dataMin()) * yAxis.scale();
    double zoffset = (zAxis.min() - zAxis.dataMin()) * zAxis.scale();

    TRACE("Axis ranges: %g %g %g\n", xAxis.range(), yAxis.range(), zAxis.range());
    TRACE("Axis scales: %g %g %g\n", xAxis.scale(), yAxis.scale(), zAxis.scale());
    TRACE("Axis min/max: %g,%g %g,%g %g,%g\n",
          xAxis.min(), xAxis.max(), 
          yAxis.min(), yAxis.max(),
          zAxis.min(), zAxis.max());
    TRACE("Axis vmin/vmax: %g,%g %g,%g %g,%g\n",
          xAxis.dataMin(), xAxis.dataMax(), 
          yAxis.dataMin(), yAxis.dataMax(),
          zAxis.dataMin(), zAxis.dataMax());
    TRACE("paspect: %g %g %g\n", paspectX, paspectY, paspectZ);
    TRACE("scale: %g %g %g\n", xscale, yscale, zscale);

    glTranslatef(-0.5f * paspectX, -0.5f * paspectY, -0.5f * paspectZ);
    glScalef(xscale, yscale, zscale);
    glTranslatef(xoffset, yoffset, zoffset);

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

        for (result = xAxis.firstMajor(iter); result; result = iter.next()) {
            float x;
            x = xAxis.map(iter.getValue());
            glVertex3f(x, 0.0f, 0.0f);
            glVertex3f(x, 1.0f, 0.0f);
            glVertex3f(x, 0.0f, 0.0f);
            glVertex3f(x, 0.0f, 1.0f + GRID_TICK);
        }
        for (result = yAxis.firstMajor(iter); result; result = iter.next()) {
            float y;
            y = yAxis.map(iter.getValue());
            glVertex3f(0.0f, y, 0.0f);
            glVertex3f(1.0f + GRID_TICK, y, 0.0f);
            glVertex3f(0.0f, y, 0.0f);
            glVertex3f(0.0f, y, 1.0f);
        }
        for (result = zAxis.firstMajor(iter); result; result = iter.next()) {
            float z;
            z = zAxis.map(iter.getValue());
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

        for (result = xAxis.firstMajor(iter); result; result = iter.next()) {
            float x;
            x = xAxis.map(iter.getValue());
            if (gluProject(x, 0.0f, 1.06f, mv, prjm, viewport, &wx, &wy, &wz)) {
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
            if (gluProject(1.06f, y, 0.0f, mv, prjm, viewport, &wx, &wy, &wz)) {
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
            if (gluProject(1.06f, 0.0f, z, mv, prjm, viewport, &wx, &wy, &wz)) {
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

void Grid::setFont(R2Fonts *font)
{
//    if (font == _font) return;
//    if (font) font->addRef();
//    if (_font) _font->unrefDelete();

    _font = font;
}

