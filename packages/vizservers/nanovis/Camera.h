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
    Camera(int startx, int starty, int w, int h,
             float loc_x, float loc_y, float loc_z);

    ~Camera()
    {}

    //move location of camera
    void x(float loc_x)
    {
        _location.x = loc_x;
    }

    float x() const
    {
        return _location.x;
    }

    void y(float loc_y)
    {
        _location.y = loc_y;
    }

    float y() const
    {
        return _location.y;
    }

    void z(float loc_z)
    {
        _location.z = loc_z;
    }

    float z() const
    {
        return _location.z;
    }

    void rotate(double *quat);

    void rotate(float angle_x, float angle_y, float angle_z);

    void fov(float fov)
    {
        _fov = fov;
    }

    float fov() const
    {
        return _fov;
    }

    void reset(const vrmath::Vector3f& bboxMin,
               const vrmath::Vector3f& bboxMax,
               bool resetOrientation = false);

    void setClippingRange(float near, float far)
    {
        _near = near;
        _far = far;
    }

    void resetClippingRange(const vrmath::Vector3f& bboxMin,
                            const vrmath::Vector3f& bboxMax);

    void setScreenSize(int sx, int sy, int w, int h)
    {
        _width = w;
        _height = h;
        _startX = sx;
        _startY = sy;
    }

    /**
     * \brief Make the camera setting active, this has to be
     * called before drawing things
     */
    void initialize();

    void print() const;

private:
    void getUpDirMatrix(vrmath::Matrix4x4d& upMat);

    /// Location of the camera in the scene
    vrmath::Vector3f _location;
    /// Camera view matrix (orientation only, no translation)
    vrmath::Matrix4x4d _cameraMatrix;
    /// Field of view (vertical angle in degrees)
    float _fov;
    /// Near, far z clipping
    float _near, _far;

    /// screen width
    int _width;
    /// screen height
    int _height;
    int _startX;
    int _startY;
};

}

#endif
