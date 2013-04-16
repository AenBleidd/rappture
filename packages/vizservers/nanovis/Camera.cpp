/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  Authors:
 *    Wei Qiao <qiaow@purdue.edu>
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
#include <vrmath/BBox.h>

#include "nanovis.h"
#include "Camera.h"
#include "Trace.h"

using namespace nv;
using namespace vrmath;

static inline double deg2rad(double deg)
{
    return ((deg * M_PI) / 180.);
}

static inline double rad2deg(double rad)
{
    return ((rad * 180.) / M_PI);
}

Camera::Camera(int x, int y, int width, int height) :
    _updir(Y_POS),
    _zoomRatio(1),
    _position(0, 0, 2.5),
    _focalPoint(0, 0, 0),
    _mvDirty(true),
    _fov(30.0),
    _near(0.1),
    _far(50.0)
{
    _viewport[0] = x;
    _viewport[1] = y;
    _viewport[2] = width;
    _viewport[3] = height;
    _pan[0] = 0;
    _pan[1] = 0;
}

void
Camera::pan(float x, float y, bool absolute)
{
    y = -y;
    if (absolute) {
        double panAbs[2];
        panAbs[0] = x;
        panAbs[1] = y;
        x -= _pan[0];
        y -= _pan[1];
        _pan[0] = panAbs[0];
        _pan[1] = panAbs[1];
    } else {
        _pan[0] += x;
        _pan[1] += y;
    }

    if (x != 0.0 || y != 0.0) {
        Vector3f worldFocalPoint;
        worldToWindowCoords(_focalPoint.x, _focalPoint.y, _focalPoint.z, 
                            worldFocalPoint);
        float focalDepth = worldFocalPoint.z;

        Vector3f newPickPoint, oldPickPoint, motionVector;
        windowToWorldCoords((x * 2. + 1.) * (float)_viewport[2] / 2.0,
                            (y * 2. + 1.) * (float)_viewport[3] / 2.0,
                            focalDepth, 
                            newPickPoint);

        windowToWorldCoords((float)_viewport[2] / 2.0,
                            (float)_viewport[3] / 2.0,
                            focalDepth, 
                            oldPickPoint);

        // Camera motion is reversed
        motionVector = oldPickPoint - newPickPoint;

        _focalPoint += motionVector;
        _position += motionVector;

        _mvDirty = true;
    }
}

void
Camera::zoom(float z, bool absolute)
{
    if (absolute) {
        // Compute relative zoom
        float zAbs = z;
        z *= 1.0/_zoomRatio;
        _zoomRatio = zAbs;
    } else {
        _zoomRatio *= z;
    }
    float distance = getDistance() / z;
    Vector3f dir = getViewPlaneNormal();
    dir = dir.scale(distance);
    _position = _focalPoint + dir;
    _mvDirty = true;
}

const Matrix4x4d&
Camera::getUpDirMatrix() const
{
    return _updirMatrix;
}

void
Camera::setUpDirMatrix(AxisDirection dir)
{
    _updir = dir;

    switch (_updir) {
    case X_POS: {
        _updirMatrix.makeRotation(0, 0, 1, deg2rad(90));
        Matrix4x4d tmp;
        tmp.makeRotation(1, 0, 0, deg2rad(90));
        _updirMatrix.multiply(tmp);
    }
        break;
    case Y_POS:
        _updirMatrix.makeIdentity();
        break;
    case Z_POS: {
        _updirMatrix.makeRotation(1, 0, 0, deg2rad(-90));
        Matrix4x4d tmp;
        tmp.makeRotation(0, 0, 1, deg2rad(-90));
        _updirMatrix.multiply(tmp);
    }
        break;
    case X_NEG:
        _updirMatrix.makeRotation(0, 0, 1, deg2rad(-90));
        break;
    case Y_NEG: {
        _updirMatrix.makeRotation(0, 0, 1, deg2rad(180));
        Matrix4x4d tmp;
        tmp.makeRotation(0, 1, 0, deg2rad(-90));
        _updirMatrix.multiply(tmp);
    }
        break;
    case Z_NEG:
        _updirMatrix.makeRotation(1, 0, 0, deg2rad(90));
        break;
    }
}

/**
 * \brief Reset zoom to include extents
 */
void
Camera::reset(const Vector3f& bboxMin,
              const Vector3f& bboxMax,
              bool resetOrientation)
{
    TRACE("Enter");

    if (resetOrientation) {
        _rotationMatrix.makeIdentity();
    }

    BBox bbox(bboxMin, bboxMax);
    Vector3f center = bbox.getCenter();
    _focalPoint.set(center.x, center.y, center.z);

    Matrix4x4d mat;
    mat.makeTranslation(-_focalPoint);
    mat.multiply(_updirMatrix, mat); // premult
    mat.multiply(_rotationMatrix, mat); // premult

    bbox.transform(bbox, mat);

    Vector3f emin = bbox.min;
    Vector3f emax = bbox.max;

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
    double winAspect = (double)_viewport[2]/(double)_viewport[3];
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

    Vector3f viewPlaneNormal(0, 0, 1);
    mat.transpose();
    viewPlaneNormal = mat.transformVec(viewPlaneNormal);
    _position = _focalPoint + viewPlaneNormal * distance;

    TRACE("win aspect: %g scene aspect: %g", winAspect, sceneAspect);
    TRACE("vpn: %g, %g, %g", viewPlaneNormal.x, viewPlaneNormal.y, viewPlaneNormal.z);
    TRACE("c: %g,%g,%g, d: %g", center.x, center.y, center.z, distance);
    TRACE("pos: %g, %g, %g", _position.x, _position.y, _position.z);
    TRACE("near: %g, far: %g", _near, _far);

    _zoomRatio = 1.0;
    _pan[0] = 0;
    _pan[1] = 0;
    _mvDirty = true;

    initialize();
    resetClippingRange(bboxMin, bboxMax);
}

/**
 * \brief Reset near and far planes based on given world space
 * bounding box
 *
 * This method is based on VTK's renderer class implementation
 * The idea is to plug the bounding box corners into the view 
 * plane equation to find the min and max distance of the scene
 * to the view plane.
 */
void
Camera::resetClippingRange(const Vector3f& bboxMin, const Vector3f& bboxMax)
{
    BBox bbox(bboxMin, bboxMax);

    // Set up view plane equation at camera position
    Vector3f vpn = getViewPlaneNormal();
    double a, b, c, d, dist;
    a = -vpn.x;
    b = -vpn.y;
    c = -vpn.z;
    d = -(a*_position.x + b*_position.y + c*_position.z);

    // Now compute distance of bounding box corners to view plane

    // Set starting limits
    _near = a * bbox.min.x + b * bbox.min.y + c * bbox.min.z + d;
    _far = 1e-18;

    // Iterate over the 8 bbox corners
    for (int k = 0; k < 2; k++) {
        for (int j = 0; j < 2; j++) {
            for (int i = 0; i < 2; i++) {
                dist =
                    (a * ((i == 0) ? bbox.min.x : bbox.max.x)) +
                    (b * ((j == 0) ? bbox.min.y : bbox.max.y)) +
                    (c * ((k == 0) ? bbox.min.z : bbox.max.z)) + d;
                _near = (dist < _near) ? dist : _near;
                _far =  (dist > _far)  ? dist : _far;
            }
        }
    }

    // Check for near plane behind the camera
    if (_near < 0) {
        _near = 0;
    }

    // Extend the bounds a bit 
    _near = 0.99 * _near - (_far - _near) * 0.5;
    _far  = 1.01 * _far  + (_far - _near) * 0.5;

    // Ensure near is closer than far
    _near = (_near >= _far) ? (0.01 * _far) : _near;

    // Limit clip range to make best use of z buffer precision
    if (_near < .001 * _far)
        _near = .001 * _far;

    TRACE("Resetting camera clipping range to: near: %g, far: %g", _near, _far);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(_fov, 
                   (GLdouble)_viewport[2]/(GLdouble)_viewport[3], 
                   _near, _far);
    glMatrixMode(GL_MODELVIEW);

    print();
}

void 
Camera::initialize()
{
    TRACE("Enter");
    print();

    glViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(_fov, 
                   (GLdouble)_viewport[2]/(GLdouble)_viewport[3], 
                   _near, _far);

    glMatrixMode(GL_MODELVIEW);

    computeModelViewMatrix();
    glLoadMatrixd((const GLdouble *)_modelViewMatrix.get());
}

void Camera::windowToWorldCoords(double x, double y, double z, Vector3f& objPos)
{
    Matrix4x4d proj;
    getProjectionMatrix(proj);
    GLdouble outX, outY, outZ;
    gluUnProject(x, y, z,
                 (GLdouble *)getModelViewMatrix().get(),
                 (GLdouble *)proj.get(),
                 (GLint *)_viewport,
                 &outX, &outY, &outZ);
    objPos.set(outX, outY, outZ);
}

void Camera::worldToWindowCoords(double x, double y, double z, Vector3f& winPos)
{
    Matrix4x4d proj;
    getProjectionMatrix(proj);
    GLdouble outX, outY, outZ;
    gluProject(x, y, z,
               (GLdouble *)getModelViewMatrix().get(),
               (GLdouble *)proj.get(),
               (GLint *)_viewport,
               &outX, &outY, &outZ);
    winPos.set(outX, outY, outZ);
}

void Camera::computeModelViewMatrix()
{
    if (_mvDirty) {
        _modelViewMatrix.makeTranslation(0, 0, -getDistance());
        _modelViewMatrix.multiply(_rotationMatrix);
        _modelViewMatrix.multiply(_updirMatrix);
        Matrix4x4d mat;
        mat.makeTranslation(-_focalPoint);
        _modelViewMatrix.multiply(mat);
        _mvDirty = false;
    }
}

void Camera::getProjectionMatrix(Matrix4x4d& mat)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(_fov, 
                   (GLdouble)_viewport[2]/(GLdouble)_viewport[3], 
                   _near, _far);
    glGetDoublev(GL_PROJECTION_MATRIX, (GLdouble *)mat.get());
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void Camera::orient(double *quat)
{
    Quaternion q(quat[0], quat[1], quat[2], quat[3]);
    Rotation rot;
    rot.set(q);
    _rotationMatrix.makeRotation(rot);
    _rotationMatrix.transpose();
    _mvDirty = true;
    computeModelViewMatrix();

    Vector3f viewPlaneNormal(0, 0, 1);
    Matrix4x4d mat = getModelViewMatrix();
    mat.transpose();
    viewPlaneNormal = mat.transformVec(viewPlaneNormal);
    TRACE("vpn: %g %g %g", viewPlaneNormal.x, viewPlaneNormal.y, viewPlaneNormal.z);
    _position = _focalPoint + viewPlaneNormal * getDistance();

    TRACE("Set rotation to quat: %g %g %g %g",
          quat[0], quat[1], quat[2], quat[3]);
}

void Camera::orient(float angleX, float angleY, float angleZ)
{
    angleX = -angleX;
    angleY = angleY - 180.;

    _rotationMatrix.makeRotation(1, 0, 0, deg2rad(angleX));
    Matrix4x4d mat;
    mat.makeRotation(0, 1, 0, deg2rad(angleY));
    _rotationMatrix.multiply(mat);
    mat.makeRotation(0, 0, 1, deg2rad(angleZ));
    _rotationMatrix.multiply(mat);
    _mvDirty = true;
    computeModelViewMatrix();

    Vector3f viewPlaneNormal(0, 0, 1);
    mat = getModelViewMatrix();
    mat.transpose();
    viewPlaneNormal = mat.transformVec(viewPlaneNormal);
    TRACE("vpn: %g %g %g", viewPlaneNormal.x, viewPlaneNormal.y, viewPlaneNormal.z);
    _position = _focalPoint + viewPlaneNormal * getDistance();

    TRACE("Set rotation to angles: %g %g %g",
          angleX, angleY, angleZ);
}

void Camera::print() const
{
    TRACE("x: %d y: %d w: %d h: %d",
          _viewport[0], _viewport[1], _viewport[2], _viewport[3]);
    TRACE("fov: %g near: %g far: %g", _fov, _near, _far);
    TRACE("fp: %g, %g, %g",
          _focalPoint.x, _focalPoint.y, _focalPoint.z);
    TRACE("pos: %g, %g, %g",
          _position.x, _position.y, _position.z);
    TRACE("Rotation matrix: ");
    _rotationMatrix.print();
    TRACE("Modelview matrix: ");
    _modelViewMatrix.print();
}
