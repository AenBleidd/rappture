/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  Authors:
 *    Wei Qiao <qiaow@purdue.edu>
 */
#ifndef NV_CAMERA_H
#define NV_CAMERA_H

#include <vrmath/Matrix4x4d.h>
#include <vrmath/Vector3f.h>

#include "config.h"

namespace nv {

class Camera
{
public:
    enum AxisDirection {
        X_POS = 1,
        Y_POS = 2,
        Z_POS = 3,
        X_NEG = -1,
        Y_NEG = -2,
        Z_NEG = -3
    };

    Camera(int x, int y, int width, int height);

    ~Camera()
    {}

    void setUpdir(AxisDirection dir)
    {
         setUpDirMatrix(dir);
        _mvDirty = true;
    }

    void setPosition(const vrmath::Vector3f& pos)
    {
        _position = pos;
        _mvDirty = true;
    }

    const vrmath::Vector3f& getPosition() const
    {
        return _position;
    }

    const vrmath::Vector3f& getFocalPoint() const
    {
        return _focalPoint;
    }

    float getDistance() const
    {
        return vrmath::Vector3f(_focalPoint - _position).length();
    }

    vrmath::Vector3f getDirectionOfProjection() const
    {
        vrmath::Vector3f dir = _focalPoint - _position;
        dir = dir.normalize();
        return dir;
    }

    vrmath::Vector3f getViewPlaneNormal() const
    {
        vrmath::Vector3f dir = _position - _focalPoint;
        dir = dir.normalize();
        return dir;
    }

    void pan(float x, float y, bool absolute = true);

    void zoom(float z, bool absolute = true);

    void orient(double *quat);

    void orient(float angleX, float angleY, float angleZ);

    void setFov(float fov)
    {
        _fov = fov;
    }

    float getFov() const
    {
        return _fov;
    }

    void reset(const vrmath::Vector3f& bboxMin,
               const vrmath::Vector3f& bboxMax,
               bool resetOrientation = false);

    void resetClippingRange(const vrmath::Vector3f& bboxMin,
                            const vrmath::Vector3f& bboxMax);

    void setViewport(int x, int y, int width, int height)
    {
        _viewport[0] = x;
        _viewport[1] = y;
        _viewport[2] = width;
        _viewport[3] = height;
    }

    /**
     * \brief Make the camera setting active, this has to be
     * called before drawing things
     */
    void initialize();

    void print() const;

    vrmath::Matrix4x4d& getModelViewMatrix()
    {
        computeModelViewMatrix();
        return _modelViewMatrix;
    }

    void getProjectionMatrix(vrmath::Matrix4x4d& mat);

    const vrmath::Matrix4x4d& getUpDirMatrix() const;

    void windowToWorldCoords(double x, double y, double z, vrmath::Vector3f& objPos);

    void worldToWindowCoords(double x, double y, double z, vrmath::Vector3f& winPos);

private:
    void computeModelViewMatrix();

    void setUpDirMatrix(AxisDirection dir);

    AxisDirection _updir;

    double _zoomRatio;
    float _pan[2];
    /// Position of the camera in the scene
    vrmath::Vector3f _position;
    vrmath::Vector3f _focalPoint;

    /// Model transform for z-up scene
    vrmath::Matrix4x4d _updirMatrix;
    /// Camera orientation
    vrmath::Matrix4x4d _rotationMatrix;
    /// Full camera matrix
    bool _mvDirty;
    vrmath::Matrix4x4d _modelViewMatrix;

    /// Field of view (vertical angle in degrees)
    float _fov;
    /// Near, far z clipping
    float _near, _far;

    // x, y, width, height
    int _viewport[4];
 };

}

#endif
