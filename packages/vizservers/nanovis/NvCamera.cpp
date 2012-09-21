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

#include <GL/glew.h>
#include <GL/glu.h>

#include <vrmath/vrQuaternion.h>
#include <vrmath/vrRotation.h>
#include <vrmath/vrMatrix4x4f.h>

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
                   float loc_x, float loc_y, float loc_z, 
                   float target_x, float target_y, float target_z,
                   float angle_x, float angle_y, float angle_z) :
    _location(loc_x, loc_y, loc_z),
    _target(target_x, target_y, target_z),
    _angle(angle_x, angle_y, angle_z),
    _width(w),
    _height(h),
    _startX(startx),
    _startY(starty)
{
}

void 
NvCamera::initialize()
{
    TRACE("camera: %d, %d\n", _width, _height);
    glViewport(_startX, _startY, _width, _height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(30, 
                   (GLdouble)(_width - _startX)/(GLdouble)(_height - _startY), 
                   0.1, 50.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

#ifndef OLD_CAMERA
    glTranslatef(-_location.x, -_location.y, -_location.z);
    glMultMatrixf((const GLfloat *)_cameraMatrix.get());
#else

    gluLookAt(_location.x, _location.y, _location.z,
              _target.x, _target.y, _target.z,
              0., 1., 0.);

    if (_angle.x != 0.0f)
        glRotated(_angle.x, 1., 0., 0.);
    if (_angle.y != 0.0f)
        glRotated(_angle.y, 0., 1., 0.);
    if (_angle.z != 0.0f)
        glRotated(_angle.z, 0., 0., 1.);
#endif
}

void NvCamera::rotate(double *quat)
{
    vrQuaternion q(quat[0], quat[1], quat[2], quat[3]);
    vrRotation rot;
    rot.set(q);
    _cameraMatrix.makeRotation(rot);
    _cameraMatrix.transpose();
    _angle.set(0, 0, 0);
    TRACE("Set rotation to quat: %g %g %g %g\n",
          quat[0], quat[1], quat[2], quat[3]);
}

void NvCamera::rotate(float angle_x, float angle_y, float angle_z)
{
#ifdef OLD_CAMERA
    _angle = Vector3(angle_x, angle_y, angle_z);
#else
    angle_x = -angle_x;
    angle_y = angle_y - 180.;
    _angle = Vector3(angle_x, angle_y, angle_z);

    _cameraMatrix.makeRotation(1, 0, 0, deg2rad(_angle.x));
    vrMatrix4x4f mat;
    mat.makeRotation(0, 1, 0, deg2rad(_angle.y));
    _cameraMatrix.multiply(mat);
    mat.makeRotation(0, 0, 1, deg2rad(_angle.z));
    _cameraMatrix.multiply(mat);
    //_cameraMatrix.transpose();
#endif
    TRACE("Set rotation to angles: %g %g %g\n",
          _angle.x, _angle.y, _angle.z);
}

void NvCamera::rotate(const Vector3& angle)
{ 
#ifdef OLD_CAMERA
    _angle = angle;
#else
    _angle.x = -angle.x;
    _angle.y = angle.y - 180.;
    _angle.z = angle.z;

    _cameraMatrix.makeRotation(1, 0, 0, deg2rad(_angle.x));
    vrMatrix4x4f mat;
    mat.makeRotation(0, 1, 0, deg2rad(_angle.y));
    _cameraMatrix.multiply(mat);
    mat.makeRotation(0, 0, 1, deg2rad(_angle.z));
    _cameraMatrix.multiply(mat);
    //_cameraMatrix.transpose();
#endif
    TRACE("Set rotation to angles: %g %g %g\n",
          _angle.x, _angle.y, _angle.z);
}
