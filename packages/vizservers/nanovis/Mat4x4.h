/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Mat4x4.h: Mat4x4 class
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
#ifndef _MAT4X4_H_
#define _MAT4X4_H_

#include "Vector4.h"

class Mat4x4
{
public:
    float m[16];        //row by row

    Mat4x4()
    {}

    Mat4x4(float *vals);

    void print() const;

    Mat4x4 inverse() const;

    Mat4x4 transpose() const;

    Vector4 multiplyRowVector(const Vector4& v) const;

    Vector4 transform(const Vector4& v) const;

    Mat4x4 operator*(const Mat4x4& op) const;
};

#endif
