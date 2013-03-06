/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * NvCamera.h : NvCamera class
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
#ifndef CAMERA_H
#define CAMERA_H

#include <vrmath/Matrix4x4f.h>

#include "config.h"
#include "Vector3.h"

class NvCamera
{
public:
    NvCamera(int startx, int starty, int w, int h,
             float loc_x, float loc_y, float loc_z);

    ~NvCamera()
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

    void setClippingRange(float near, float far)
    {
        _near = near;
        _far = far;
    }

    void setClippingRange(const Vector3& bboxMin, const Vector3& bboxMax);

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

private:
    /// Location of the camera in the scene
    Vector3 _location;
    /// Camera view matrix (orientation only, no translation)
    vrMatrix4x4f _cameraMatrix;
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

#endif
