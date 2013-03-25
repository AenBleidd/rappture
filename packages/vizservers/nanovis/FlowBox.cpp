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
    Rappture::FreeSwitches(_switches, &_sv, 0);
}

void
FlowBox::getWorldSpaceBounds(Vector3f& bboxMin,
                             Vector3f& bboxMax,
                             const Volume *vol) const
{
    bboxMin.set(FLT_MAX, FLT_MAX, FLT_MAX);
    bboxMax.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    Vector3f origin = vol->location();
    Vector3f scale = vol->getPhysicalScaling();

    Matrix4x4d mat;
    mat.makeTranslation(origin);
    Matrix4x4d mat2;
    mat2.makeScale(scale);

    mat.multiply(mat2);

    Vector3f min, max;
    min.x = vol->xAxis.min();
    min.y = vol->yAxis.min();
    min.z = vol->zAxis.min();
    max.x = vol->xAxis.max();
    max.y = vol->yAxis.max();
    max.z = vol->zAxis.max();

    float x0, y0, z0, x1, y1, z1;
    x0 = y0 = z0 = 0.0f;
    x1 = y1 = z1 = 0.0f;
    if (max.x > min.x) {
        x0 = (_sv.corner1.x - min.x) / (max.x - min.x);
        x1 = (_sv.corner2.x - min.x) / (max.x - min.x);
    }
    if (max.y > min.y) {
        y0 = (_sv.corner1.y - min.y) / (max.y - min.y);
        y1 = (_sv.corner2.y - min.y) / (max.y - min.y);
    }
    if (max.z > min.z) {
        z0 = (_sv.corner1.z - min.z) / (max.z - min.z);
        z1 = (_sv.corner2.z - min.z) / (max.z - min.z);
    }

    TRACE("Box model bounds: (%g,%g,%g) - (%g,%g,%g)",
          x0, y0, z0, x1, y1, z1);

    Vector3f modelMin(x0, y0, z0);
    Vector3f modelMax(x1, y1, z1);

    Vector4f bvert[8];
    bvert[0] = Vector4f(modelMin.x, modelMin.y, modelMin.z, 1);
    bvert[1] = Vector4f(modelMax.x, modelMin.y, modelMin.z, 1);
    bvert[2] = Vector4f(modelMin.x, modelMax.y, modelMin.z, 1);
    bvert[3] = Vector4f(modelMin.x, modelMin.y, modelMax.z, 1);
    bvert[4] = Vector4f(modelMax.x, modelMax.y, modelMin.z, 1);
    bvert[5] = Vector4f(modelMax.x, modelMin.y, modelMax.z, 1);
    bvert[6] = Vector4f(modelMin.x, modelMax.y, modelMax.z, 1);
    bvert[7] = Vector4f(modelMax.x, modelMax.y, modelMax.z, 1);

    for (int i = 0; i < 8; i++) {
        Vector4f worldVert = mat.transform(bvert[i]);
        if (worldVert.x < bboxMin.x) bboxMin.x = worldVert.x;
        if (worldVert.x > bboxMax.x) bboxMax.x = worldVert.x;
        if (worldVert.y < bboxMin.y) bboxMin.y = worldVert.y;
        if (worldVert.y > bboxMax.y) bboxMax.y = worldVert.y;
        if (worldVert.z < bboxMin.z) bboxMin.z = worldVert.z;
        if (worldVert.z > bboxMax.z) bboxMax.z = worldVert.z;
    }

    TRACE("Box world bounds: (%g,%g,%g) - (%g,%g,%g)",
          bboxMin.x, bboxMin.y, bboxMin.z,
          bboxMax.x, bboxMax.y, bboxMax.z);
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

    Vector3f origin = vol->location();
    glTranslatef(origin.x, origin.y, origin.z);

    Vector3f scale = vol->getPhysicalScaling();
    glScalef(scale.x, scale.y, scale.z);

    Vector3f min, max;
    min.x = vol->xAxis.min();
    min.y = vol->yAxis.min();
    min.z = vol->zAxis.min();
    max.x = vol->xAxis.max();
    max.y = vol->yAxis.max();
    max.z = vol->zAxis.max();

    TRACE("box is %g,%g %g,%g %g,%g",
          _sv.corner1.x, _sv.corner2.x,
          _sv.corner1.y, _sv.corner2.y,
          _sv.corner1.z, _sv.corner2.z);
    TRACE("world is %g,%g %g,%g %g,%g",
          min.x, max.x, min.y, max.y, min.z, max.z);

    float x0, y0, z0, x1, y1, z1;
    x0 = y0 = z0 = 0.0f;
    x1 = y1 = z1 = 0.0f;
    if (max.x > min.x) {
        x0 = (_sv.corner1.x - min.x) / (max.x - min.x);
        x1 = (_sv.corner2.x - min.x) / (max.x - min.x);
    }
    if (max.y > min.y) {
        y0 = (_sv.corner1.y - min.y) / (max.y - min.y);
        y1 = (_sv.corner2.y - min.y) / (max.y - min.y);
    }
    if (max.z > min.z) {
        z0 = (_sv.corner1.z - min.z) / (max.z - min.z);
        z1 = (_sv.corner2.z - min.z) / (max.z - min.z);
    }
    TRACE("box bounds: %g,%g %g,%g %g,%g",
          x0, x1, y0, y1, z0, z1);

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
