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

#include <vrmath/vrMatrix4x4f.h>

#include "config.h"
#include "Vector3.h"

class NvCamera
{
public:
    NvCamera(int startx, int starty, int w, int h,
             float loc_x, float loc_y, float loc_z, 
             float target_x, float target_y, float target_z,
             float angle_x, float angle_y, float angle_z);

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

    //move location of target
    void xAim(float x)
    {
        _target.x = x;
    }

    float xAim() const
    {
        return _target.x;
    }

    void yAim(float y)
    {
        _target.y = y;
    }

    float yAim() const
    {
        return _target.y;
    }

    void zAim(float z)
    {
        _target.z = z;
    }

    float zAim() const
    {
        return _target.z;
    }

    void rotate(double *quat);

    void rotate(float angle_x, float angle_y, float angle_z);

    void rotate(const Vector3& angle);

    Vector3 rotate() const
    { 
        return _angle;
    }

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
    /**
     * \brief Location the camera is looking at
     *
     * Location and target: two points define the line-of-sight
     */
    Vector3 _target;
    /// rotation angles of camera along x, y, z
    Vector3 _angle;

    vrMatrix4x4f _cameraMatrix;

    /// screen width
    int _width;
    /// screen height
    int _height;
    int _startX;
    int _startY;
};

#endif
