/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Authors:
 *   Wei Qiao <qiaow@purdue.edu>
 *   Insoo Woo <iwoo@purdue.edu>
 *   George A. Howlett <gah@purdue.edu>
 *   Leif Delgass <ldelgass@purdue.edu>
 */

#include <float.h>
#include <assert.h>

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>
#include <vrmath/Matrix4x4d.h>

#include "FlowBox.h"
#include "Volume.h"
#include "Trace.h"

using namespace nv;
using namespace vrmath;

FlowBox::FlowBox(const char *name) :
    _name(name)
{
    _sv.isHidden = false;
    _sv.corner1.x = 0.0f;
    _sv.corner1.y = 0.0f;
    _sv.corner1.z = 0.0f;
    _sv.corner2.x = 1.0f;
    _sv.corner2.y = 1.0f;
    _sv.corner2.z = 1.0f;
    _sv.lineWidth = 1.2f;
    _sv.color.r = _sv.color.b = _sv.color.g = _sv.color.a = 1.0f;
}

FlowBox::~FlowBox()
{
    TRACE("Freeing switches");
    FreeSwitches(_switches, &_sv, 0);
}

void
FlowBox::getBounds(Vector3f& bboxMin,
                   Vector3f& bboxMax) const
{
    bboxMin.set(_sv.corner1.x, _sv.corner1.y, _sv.corner1.z);
    bboxMax.set(_sv.corner2.x, _sv.corner2.y, _sv.corner2.z);
}

void 
FlowBox::render(Volume *vol)
{
    TRACE("Box: '%s'", _name.c_str());

    glPushAttrib(GL_ENABLE_BIT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    TRACE("box bounds %g,%g %g,%g %g,%g",
          _sv.corner1.x, _sv.corner2.x,
          _sv.corner1.y, _sv.corner2.y,
          _sv.corner1.z, _sv.corner2.z);

    float x0, y0, z0, x1, y1, z1;

    x0 = _sv.corner1.x;
    x1 = _sv.corner2.x;
    y0 = _sv.corner1.y;
    y1 = _sv.corner2.y;
    z0 = _sv.corner1.z;
    z1 = _sv.corner2.z;

    glColor4d(_sv.color.r, _sv.color.g, _sv.color.b, _sv.color.a);
    glLineWidth(_sv.lineWidth);
    glBegin(GL_LINE_LOOP); 
    {
        glVertex3d(x0, y0, z0);
        glVertex3d(x1, y0, z0);
        glVertex3d(x1, y1, z0);
        glVertex3d(x0, y1, z0);
    }
    glEnd();
    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x0, y0, z1);
        glVertex3d(x1, y0, z1);
        glVertex3d(x1, y1, z1);
        glVertex3d(x0, y1, z1);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x0, y0, z0);
        glVertex3d(x0, y0, z1);
        glVertex3d(x0, y1, z1);
        glVertex3d(x0, y1, z0);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x1, y0, z0);
        glVertex3d(x1, y0, z1);
        glVertex3d(x1, y1, z1);
        glVertex3d(x1, y1, z0);
    }
    glEnd();

    glPopMatrix();
    glPopAttrib();

    assert(CheckGL(AT));
}
