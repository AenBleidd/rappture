/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <vrmath/Vector3f.h>

#include "OrientationIndicator.h"

using namespace vrmath;
using namespace nv;

OrientationIndicator::OrientationIndicator() :
    _rep(ARROWS),
    _visible(true),
    _lineWidth(1.f),
    _quadric(gluNewQuadric()),
    _position(0,0,0),
    _scale(1,1,1)
{
}

OrientationIndicator::~OrientationIndicator()
{
    gluDeleteQuadric((GLUquadric *)_quadric);
}

void OrientationIndicator::setRepresentation(Representation rep)
{
    _rep = rep;
}

void OrientationIndicator::render()
{
    if (!_visible)
        return;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(_position.x, _position.y, _position.z);
    float scale = _scale.x;
    if (scale == 0 || _scale.y < scale) scale = _scale.y;
    if (scale == 0 || _scale.z < scale) scale = _scale.z;
    glScalef(scale, scale, scale);

    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_BLEND);

    switch (_rep) {
    case LINES: {
        glLineWidth(_lineWidth);
        glDisable(GL_LIGHTING);
        if (_scale.x > 0) {
            glColor3f(1, 0, 0);
            glBegin(GL_LINES);
            glVertex3f(0, 0, 0);
            glVertex3f(0.5f, 0, 0);
            glEnd();
        }
        if (_scale.y > 0) {
            glColor3f(0, 1, 0);
            glBegin(GL_LINES);
            glVertex3f(0, 0, 0);
            glVertex3f(0, 0.5f, 0);
            glEnd();
        }
        if (_scale.z > 0) {
            glColor3f(0, 0, 1);
            glBegin(GL_LINES);
            glVertex3f(0, 0, 0);
            glVertex3f(0, 0, 0.5f);
            glEnd();
        }
    }
        break;
    case ARROWS: {
        GLUquadric *qobj = (GLUquadric *)_quadric;

        int segments = 50;

        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_NORMALIZE);

        // X
        if (_scale.x > 0) {
            glColor3f(1, 0, 0);
            glPushMatrix();
            glRotatef(90, 0, 1, 0);
            gluCylinder(qobj, 0.01, 0.01, 0.3, segments, 1);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(0.3, 0., 0.);
            glRotatef(90, 0, 1, 0);
            gluCylinder(qobj, 0.02, 0.0, 0.06, segments, 1);
            glPopMatrix();
        }

        // Y
        if (_scale.y > 0) {
            glColor3f(0, 1, 0);
            glPushMatrix();
            glRotatef(-90, 1, 0, 0);
            gluCylinder(qobj, 0.01, 0.01, 0.3, segments, 1);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(0., 0.3, 0.);
            glRotatef(-90, 1, 0, 0);
            gluCylinder(qobj, 0.02, 0.0, 0.06, segments, 1);
            glPopMatrix();
        }

        // Z
        if (_scale.z > 0) {
            glColor3f(0, 0, 1);
            glPushMatrix();
            gluCylinder(qobj, 0.01, 0.01, 0.3, segments, 1);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(0., 0., 0.3);
            gluCylinder(qobj, 0.02, 0.0, 0.06, segments, 1);
            glPopMatrix();
        }
    }
        break;
    default:
        ;
    }
    
    glPopAttrib();
    glPopMatrix();
}
