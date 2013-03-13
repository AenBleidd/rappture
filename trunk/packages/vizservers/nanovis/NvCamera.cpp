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
#include <vrmath/Matrix4x4d.h>
#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>

#include "nanovis.h"
#include "NvCamera.h"
#include "Trace.h"

using namespace vrmath;

static inline double deg2rad(double deg)
{
    return ((deg * M_PI) / 180.);
}

static inline double rad2deg(double rad)
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
NvCamera::getUpDirMatrix(Matrix4x4d& upMat)
{
    switch (NanoVis::updir) {
    case NanoVis::X_POS: {
        upMat.makeRotation(0, 0, 1, deg2rad(90));
        Matrix4x4d tmp;
        tmp.makeRotation(1, 0, 0, deg2rad(90));
        upMat.multiply(tmp);
    }
        break;
    case NanoVis::Y_POS:
        upMat.makeIdentity();
        break;
    case NanoVis::Z_POS: {
        upMat.makeRotation(1, 0, 0, deg2rad(-90));
        Matrix4x4d tmp;
        tmp.makeRotation(0, 0, 1, deg2rad(-90));
        upMat.multiply(tmp);
    }
        break;
    case NanoVis::X_NEG:
        upMat.makeRotation(0, 0, 1, deg2rad(-90));
        break;
    case NanoVis::Y_NEG: {
        upMat.makeRotation(0, 0, 1, deg2rad(180));
        Matrix4x4d tmp;
        tmp.makeRotation(0, 1, 0, deg2rad(-90));
        upMat.multiply(tmp);
    }
        break;
    case NanoVis::Z_NEG:
        upMat.makeRotation(1, 0, 0, deg2rad(90));
        break;
    }
}

/**
 * \brief Reset zoom to include extents
 */
void
NvCamera::reset(const Vector3f& bboxMin,
                const Vector3f& bboxMax,
                bool resetOrientation)
{
    TRACE("Enter");

    if (resetOrientation) {
        _cameraMatrix.makeIdentity();
    }

    Vector3f center(bboxMin + bboxMax);
    center.scale(0.5);

    Matrix4x4d mat, upMat;
    getUpDirMatrix(upMat);
    mat.makeTranslation(-center);
    mat.multiply(_cameraMatrix, mat);
    mat.multiply(upMat);

    Vector3f emin(FLT_MAX, FLT_MAX, FLT_MAX), emax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    // Transform bounds by camera matrix
    Vector4f bboxEye[8];
    bboxEye[0] = Vector4f(bboxMin.x, bboxMin.y, bboxMin.z, 1);
    bboxEye[1] = Vector4f(bboxMax.x, bboxMin.y, bboxMin.z, 1);
    bboxEye[2] = Vector4f(bboxMin.x, bboxMax.y, bboxMin.z, 1);
    bboxEye[3] = Vector4f(bboxMin.x, bboxMin.y, bboxMax.z, 1);
    bboxEye[4] = Vector4f(bboxMax.x, bboxMax.y, bboxMin.z, 1);
    bboxEye[5] = Vector4f(bboxMax.x, bboxMin.y, bboxMax.z, 1);
    bboxEye[6] = Vector4f(bboxMin.x, bboxMax.y, bboxMax.z, 1);
    bboxEye[7] = Vector4f(bboxMax.x, bboxMax.y, bboxMax.z, 1);

    for (int i = 0; i < 8; i++) {
        Vector4f eyeVert = mat.transform(bboxEye[i]);
        if (eyeVert.x < emin.x) emin.x = eyeVert.x;
        if (eyeVert.x > emax.x) emax.x = eyeVert.x;
        if (eyeVert.y < emin.y) emin.y = eyeVert.y;
        if (eyeVert.y > emax.y) emax.y = eyeVert.y;
        if (eyeVert.z < emin.z) emin.z = eyeVert.z;
        if (eyeVert.z > emax.z) emax.z = eyeVert.z;
    }

    TRACE("Eye bounds: (%g,%g,%g) - (%g,%g,%g)",
          emin.x, emin.y, emin.z,
          emax.x, emax.y, emax.z);

    double bwidth = emax.x - emin.x;
    double bheight = emax.y - emin.y;
    double bdepth = emax.z - emin.z;

    TRACE("bwidth: %g, bheight: %g, bdepth: %g", bwidth, bheight, bdepth);

    double angle = deg2rad(_fov);
    double distance;

    // Deal with vertical aspect window
    double winAspect = (double)(_width - _startX)/(double)(_height - _startY);
    double sceneAspect = 1.0;;
    if (bheight > 0.0)
        sceneAspect = bwidth / bheight;

    if (sceneAspect >= winAspect) {
        angle = 2.0 * atan(tan(angle*0.5)*winAspect);
        _near = bwidth / (2.0 * tan(angle*0.5));
    } else {
        _near = bheight / (2.0 * tan(angle*0.5));
    }

    distance = _near + bdepth * 0.5;
    _far = _near + bdepth;

    _location.set(center.x, center.y, center.z + distance);

    TRACE("win aspect: %g scene aspect: %g", winAspect, sceneAspect);
    TRACE("c: %g,%g,%g, d: %g", center.x, center.y, center.z, distance);
    TRACE("near: %g, far: %g", _near, _far);

    initialize();
    resetClippingRange(bboxMin, bboxMax);
}

void
NvCamera::resetClippingRange(const Vector3f& bboxMin, const Vector3f& bboxMax)
{
    Vector3f emin(bboxMin.x, bboxMin.y, bboxMin.z), emax(bboxMax.x, bboxMax.y, bboxMax.z);

    Vector3f center(emin + emax);
    center.scale(0.5);

    // Compute the radius of the enclosing sphere,
    // which is half the bounding box diagonal
    Vector3f diagonal(emax - emin);
    double radius = diagonal.length() * 0.5;

    // If we have just a single point, pick a radius of 1.0
    radius = (radius == 0) ? 1.0 : radius;

    TRACE("c: %g,%g,%g, r: %g cam z: %g", center.x, center.y, center.z, radius, _location.z);

    _near = _location.z - radius;
    _far = _location.z + radius;

    if (_near < 0.0) {
        _near = 0.001;
    }
    if (_far < 0.0) {
        _far = 1.0;
    }

    TRACE("Resetting camera clipping range to: near: %g, far: %g", _near, _far);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(_fov, 
                   (GLdouble)(_width - _startX)/(GLdouble)(_height - _startY), 
                   _near, _far);
    glMatrixMode(GL_MODELVIEW);

    print();
}

void 
NvCamera::initialize()
{
    TRACE("Enter");
    print();

    glViewport(_startX, _startY, _width, _height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(_fov, 
                   (GLdouble)(_width - _startX)/(GLdouble)(_height - _startY), 
                   _near, _far);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(-_location.x, -_location.y, -_location.z);
    glMultMatrixd((const GLdouble *)_cameraMatrix.get());
}

void NvCamera::rotate(double *quat)
{
    Quaternion q(quat[0], quat[1], quat[2], quat[3]);
    Rotation rot;
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
    Matrix4x4d mat;
    mat.makeRotation(0, 1, 0, deg2rad(angleY));
    _cameraMatrix.multiply(mat);
    mat.makeRotation(0, 0, 1, deg2rad(angleZ));
    _cameraMatrix.multiply(mat);
    //_cameraMatrix.transpose();

    TRACE("Set rotation to angles: %g %g %g",
          angleX, angleY, angleZ);
}

void NvCamera::print() const
{
    TRACE("x: %d y: %d w: %d h: %d", _startX, _startY, _width, _height);
    TRACE("loc: %g %g %g",
          _location.x, _location.y, _location.z);
    TRACE("Camera matrix: ");
    _cameraMatrix.print();
    TRACE("fov: %g near: %g far: %g", _fov, _near, _far);
}
