/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * NvCamera.cpp : NvCamera class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <stdio.h>
#include <math.h>
#include <float.h>

#include <GL/glew.h>
#include <GL/glu.h>

#include <vrmath/Quaternion.h>
#include <vrmath/Rotation.h>
#include <vrmath/Matrix4x4f.h>

#include "NvCamera.h"
#include "Trace.h"

static inline float deg2rad(float deg)
{
    return ((deg * M_PI) / 180.);
}

static inline float rad2deg(float rad)
{
    return ((rad * 180.) / M_PI);
}

NvCamera::NvCamera(int startx, int starty, int w, int h,
                   float loc_x, float loc_y, float loc_z) :
    _location(loc_x, loc_y, loc_z),
    _fov(30.0),
    _near(0.1),
    _far(50.0),
    _width(w),
    _height(h),
    _startX(startx),
    _startY(starty)
{
}

void
NvCamera::setClippingRange(const Vector3& bboxMin, const Vector3& bboxMax)
{
    // Transform bounds by modelview matrix
    GLfloat mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    Mat4x4 modelView = Mat4x4(mv);
    Vector4 bboxEye[8];
    bboxEye[0] = Vector4(bboxMin.x, bboxMin.y, bboxMin.z, 1);
    bboxEye[1] = Vector4(bboxMax.x, bboxMin.y, bboxMin.z, 1);
    bboxEye[2] = Vector4(bboxMin.x, bboxMax.y, bboxMin.z, 1);
    bboxEye[3] = Vector4(bboxMin.x, bboxMin.y, bboxMax.z, 1);
    bboxEye[4] = Vector4(bboxMax.x, bboxMax.y, bboxMin.z, 1);
    bboxEye[5] = Vector4(bboxMax.x, bboxMin.y, bboxMax.z, 1);
    bboxEye[6] = Vector4(bboxMin.x, bboxMax.y, bboxMax.z, 1);
    bboxEye[7] = Vector4(bboxMax.x, bboxMax.y, bboxMax.z, 1);
    double zMin = DBL_MAX;
    double zMax = -DBL_MAX;
    for (int i = 0; i < 8; i++) {
        Vector4 eyeVert = modelView.transform(bboxEye[i]);
        if (eyeVert.z < zMin) zMin = eyeVert.z;
        if (eyeVert.z > zMax) zMax = eyeVert.z;
    }
    if (zMax > 0.0) {
        zMax = -.001;
    }
    if (zMin > 0.0) {
        zMin = -50.0;
    }
    _near = -zMax;
    _far = -zMin;

    TRACE("Resetting camera clipping range to: near: %g, far: %g", _near, _far);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(_fov, 
                   (GLdouble)(_width - _startX)/(GLdouble)(_height - _startY), 
                   _near, _far);
    glMatrixMode(GL_MODELVIEW);
}

void 
NvCamera::initialize()
{
    TRACE("camera: %d, %d", _width, _height);
    glViewport(_startX, _startY, _width, _height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(_fov, 
                   (GLdouble)(_width - _startX)/(GLdouble)(_height - _startY), 
                   _near, _far);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(-_location.x, -_location.y, -_location.z);
    glMultMatrixf((const GLfloat *)_cameraMatrix.get());
}

void NvCamera::rotate(double *quat)
{
    vrQuaternion q(quat[0], quat[1], quat[2], quat[3]);
    vrRotation rot;
    rot.set(q);
    _cameraMatrix.makeRotation(rot);
    _cameraMatrix.transpose();
    TRACE("Set rotation to quat: %g %g %g %g",
          quat[0], quat[1], quat[2], quat[3]);
}

void NvCamera::rotate(float angleX, float angleY, float angleZ)
{
    angleX = -angleX;
    angleY = angleY - 180.;

    _cameraMatrix.makeRotation(1, 0, 0, deg2rad(angleX));
    vrMatrix4x4f mat;
    mat.makeRotation(0, 1, 0, deg2rad(angleY));
    _cameraMatrix.multiply(mat);
    mat.makeRotation(0, 0, 1, deg2rad(angleZ));
    _cameraMatrix.multiply(mat);
    //_cameraMatrix.transpose();

    TRACE("Set rotation to angles: %g %g %g",
          angleX, angleY, angleZ);
}
