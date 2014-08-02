/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vrmath/Matrix4x4d.h>
#include <vrmath/Vector3f.h>
#include <vrmath/Rotation.h>

using namespace vrmath;

void Matrix4x4d::makeIdentity()
{
    _data[1] = _data[2] = _data[3] = _data[4] =
        _data[6] = _data[7] = _data[8] = _data[9] =
        _data[11] = _data[12] = _data[13] = _data[14] = 0.0;
    _data[0] = _data[5] = _data[10] = _data[15] = 1.0;
}

void Matrix4x4d::makeTranslation(const Vector3f& translation)
{
    _data[1] = _data[2] = _data[3] = _data[4] =
        _data[6] = _data[7] = _data[8] = _data[9] =
        _data[11] = 0.0;
    _data[0] = _data[5] = _data[10] = _data[15] = 1.0;
    _data[12] = translation.x;
    _data[13] = translation.y;
    _data[14] = translation.z;
}

void Matrix4x4d::makeTranslation(double x, double y, double z)
{
    _data[1] = _data[2] = _data[3] = _data[4] =
        _data[6] = _data[7] = _data[8] = _data[9] =
        _data[11] = 0.0;
    _data[0] = _data[5] = _data[10] = _data[15] = 1.0;
    _data[12] = x;
    _data[13] = y;
    _data[14] = z;
}

void Matrix4x4d::makeRotation(const Rotation& rotation)
{
    if (rotation.getAngle() == 0.0 || 
        (rotation.getX() == 0.0 &&
         rotation.getY() == 0.0 &&
         rotation.getZ() == 0.0)) {
        makeIdentity();
        return;
    }

    double xAxis = rotation.getX();
    double yAxis = rotation.getY();
    double zAxis = rotation.getZ();
    double invLen = 1.0 / sqrt(xAxis * xAxis + yAxis * yAxis + zAxis * zAxis);
    double cosine = cos(rotation.getAngle());
    double sine = sin(rotation.getAngle());

    xAxis *= invLen;
    yAxis *= invLen;
    zAxis *= invLen;

    double oneMinusCosine = 1.0 - cosine;

    _data[3] = _data[7] = _data[11] = _data[12] = _data[13] = _data[14] = 0.0;
    _data[15] = 1.0;

    _data[0] = xAxis * xAxis * oneMinusCosine + cosine;
    _data[1] = yAxis * xAxis * oneMinusCosine + zAxis * sine;
    _data[2] = xAxis * zAxis * oneMinusCosine - yAxis * sine;

    _data[4] = xAxis * yAxis * oneMinusCosine - zAxis * sine;
    _data[5] = yAxis * yAxis * oneMinusCosine + cosine;
    _data[6] = yAxis * zAxis * oneMinusCosine + xAxis * sine;

    _data[8] = xAxis * zAxis * oneMinusCosine + yAxis * sine;
    _data[9] = yAxis * zAxis * oneMinusCosine - xAxis * sine;
    _data[10] = zAxis * zAxis * oneMinusCosine + cosine;
}

void Matrix4x4d::makeRotation(double xAxis, double yAxis, double zAxis, double angle)
{
    if (angle == 0.0 ||
        (xAxis == 0.0 &&
         yAxis == 0.0 &&
         zAxis == 0.0)) {
        makeIdentity();
        return;
    }

    double invLen = 1.0f / sqrt(xAxis * xAxis + yAxis * yAxis + zAxis * zAxis);
    double cosine = cos(angle);
    double sine = sin(angle);

    xAxis *= invLen;
    yAxis *= invLen;
    zAxis *= invLen;

    float oneMinusCosine = 1.0 - cosine;

    _data[3] = _data[7] = _data[11] = _data[12] = _data[13] = _data[14] = 0.0;
    _data[15] = 1.0;

    _data[0] = xAxis * xAxis * oneMinusCosine + cosine;
    _data[1] = yAxis * xAxis * oneMinusCosine + zAxis * sine;
    _data[2] = xAxis * zAxis * oneMinusCosine - yAxis * sine;

    _data[4] = xAxis * yAxis * oneMinusCosine - zAxis * sine;
    _data[5] = yAxis * yAxis * oneMinusCosine + cosine;
    _data[6] = yAxis * zAxis * oneMinusCosine + xAxis * sine;

    _data[8] = xAxis * zAxis * oneMinusCosine + yAxis * sine;
    _data[9] = yAxis * zAxis * oneMinusCosine - xAxis * sine;
    _data[10] = zAxis * zAxis * oneMinusCosine + cosine;
}

void Matrix4x4d::makeScale(const Vector3f& scale)
{
    _data[1] = _data[2] = _data[3] = _data[4] =
        _data[6] = _data[7] = _data[8] = _data[9] =
        _data[11] = 0.0f;

    _data[0] = scale.x;
    _data[5] = scale.y;
    _data[10] = scale.z;

    _data[15] = 1.0f;
    _data[12] = _data[13] = _data[14] = 0.0f;
}

void Matrix4x4d::makeScale(double x, double y, double z)
{
    _data[1] = _data[2] = _data[3] = _data[4] =
        _data[6] = _data[7] = _data[8] = _data[9] =
        _data[11] = 0.0f;

    _data[0] = x;
    _data[5] = y;
    _data[10] = z;

    _data[15] = 1.0f;
    _data[12] = _data[13] = _data[14] = 0.0f;
}

Vector4f
Matrix4x4d::transform(const Vector4f& v) const
{
    Vector4f ret;
    ret.x = float(_data[0] * v.x + _data[4] * v.y + _data[8] * v.z  + _data[12] * v.w);
    ret.y = float(_data[1] * v.x + _data[5] * v.y + _data[9] * v.z  + _data[13] * v.w);
    ret.z = float(_data[2] * v.x + _data[6] * v.y + _data[10] * v.z + _data[14] * v.w);
    ret.w = float(_data[3] * v.x + _data[7] * v.y + _data[11] * v.z + _data[15] * v.w);
    return ret;
}

Vector3f
Matrix4x4d::transformVec(const Vector3f& v) const
{
    Vector3f ret;
    ret.x = float(_data[0] * v.x + _data[4] * v.y + _data[8] * v.z);
    ret.y = float(_data[1] * v.x + _data[5] * v.y + _data[9] * v.z);
    ret.z = float(_data[2] * v.x + _data[6] * v.y + _data[10] * v.z);
    return ret;
}

Vector4f
Matrix4x4d::preMultiplyRowVector(const Vector4f& v) const
{
    Vector4f ret;
    ret.x = float(_data[0]  * v.x + _data[1]  * v.y + _data[2]  * v.z + _data[3]  * v.w);
    ret.y = float(_data[4]  * v.x + _data[5]  * v.y + _data[6]  * v.z + _data[7]  * v.w);
    ret.z = float(_data[8]  * v.x + _data[9]  * v.y + _data[10] * v.z + _data[11] * v.w);
    ret.w = float(_data[12] * v.x + _data[13] * v.y + _data[14] * v.z + _data[15] * v.w);
    return ret;
}

void Matrix4x4d::multiply(const Matrix4x4d& m)
{
    double mat[16];
    const double *mat2 = m._data;

    // 1 row
    mat[0] = _data[0] * mat2[0] + _data[4] * mat2[1] + 
             _data[8] * mat2[2] + _data[12] * mat2[3];
    mat[4] = _data[0] * mat2[4] + _data[4] * mat2[5] + 
             _data[8] * mat2[6] + _data[12] * mat2[7];
    mat[8] = _data[0] * mat2[8] + _data[4] * mat2[9] + 
             _data[8] * mat2[10] + _data[12] * mat2[11];
    mat[12] = _data[0] * mat2[12] + _data[4] * mat2[13] + 
              _data[8] * mat2[14] + _data[12] * mat2[15];

    // 2 row
    mat[1] = _data[1] * mat2[0] + _data[5] * mat2[1] + 
             _data[9] * mat2[2] + _data[13] * mat2[3];
    mat[5] = _data[1] * mat2[4] + _data[5] * mat2[5] + 
             _data[9] * mat2[6] + _data[13] * mat2[7];
    mat[9] = _data[1] * mat2[8] + _data[5] * mat2[9] + 
             _data[9] * mat2[10] + _data[13] * mat2[11];
    mat[13] = _data[1] * mat2[12] + _data[5] * mat2[13] + 
              _data[9] * mat2[14] + _data[13] * mat2[15];

    // 3 row
    mat[2] = _data[2] * mat2[0] + _data[6] * mat2[1] + 
             _data[10] * mat2[2] + _data[14] * mat2[3];
    mat[6] = _data[2] * mat2[4] + _data[6] * mat2[5] + 
             _data[10] * mat2[6] + _data[14] * mat2[7];
    mat[10] = _data[2] * mat2[8] + _data[6] * mat2[9] + 
              _data[10] * mat2[10] + _data[14] * mat2[11];
    mat[14] = _data[2] * mat2[12] + _data[6] * mat2[13] + 
              _data[10] * mat2[14] + _data[14] * mat2[15];

    // 4 row
    mat[3] = _data[3] * mat2[0] + _data[7] * mat2[1] + 
             _data[11] * mat2[2] + _data[15] * mat2[3];
    mat[7] = _data[3] * mat2[4] + _data[7] * mat2[5] + 
             _data[11] * mat2[6] + _data[15] * mat2[7];
    mat[11] = _data[3] * mat2[8] + _data[7] * mat2[9] + 
              _data[11] * mat2[10] + _data[15] * mat2[11];
    mat[15] = _data[3] * mat2[12] + _data[7] * mat2[13] + 
              _data[11] * mat2[14] + _data[15] * mat2[15];

    // set matrix
    set(mat);
}

void Matrix4x4d::multiply(const Matrix4x4d& m1, const Matrix4x4d& m2)
{
    if (&m1 == this && &m2 != this) {
        multiply(m2);
    } else if (&m1 != this && &m2 != this) {
        multiplyFast(m1, m2);
    } else {
        double mat[16];
        const double *mat1 = m1._data;
        const double *mat2 = m2._data;

        // 1 row
        mat[0] = mat1[0] * mat2[0] + mat1[4] * mat2[1] + 
            mat1[8] * mat2[2] + mat1[12] * mat2[3];
        mat[4] = mat1[0] * mat2[4] + mat1[4] * mat2[5] + 
            mat1[8] * mat2[6] + mat1[12] * mat2[7];
        mat[8] = mat1[0] * mat2[8] + mat1[4] * mat2[9] + 
            mat1[8] * mat2[10] + mat1[12] * mat2[11];
        mat[12] = mat1[0] * mat2[12] + mat1[4] * mat2[13] + 
            mat1[8] * mat2[14] + mat1[12] * mat2[15];

        // 2 row
        mat[1] = mat1[1] * mat2[0] + mat1[5] * mat2[1] + 
            mat1[9] * mat2[2] + mat1[13] * mat2[3];
        mat[5] = mat1[1] * mat2[4] + mat1[5] * mat2[5] + 
            mat1[9] * mat2[6] + mat1[13] * mat2[7];
        mat[9] = mat1[1] * mat2[8] + mat1[5] * mat2[9] + 
            mat1[9] * mat2[10] + mat1[13] * mat2[11];
        mat[13] = mat1[1] * mat2[12] + mat1[5] * mat2[13] + 
            mat1[9] * mat2[14] + mat1[13] * mat2[15];

        // 3 row
        mat[2] = mat1[2] * mat2[0] + mat1[6] * mat2[1] + 
            mat1[10] * mat2[2] + mat1[14] * mat2[3];
        mat[6] = mat1[2] * mat2[4] + mat1[6] * mat2[5] + 
            mat1[10] * mat2[6] + mat1[14] * mat2[7];
        mat[10] = mat1[2] * mat2[8] + mat1[6] * mat2[9] + 
            mat1[10] * mat2[10] + mat1[14] * mat2[11];
        mat[14] = mat1[2] * mat2[12] + mat1[6] * mat2[13] + 
            mat1[10] * mat2[14] + mat1[14] * mat2[15];

        // 4 row
        mat[3] = mat1[3] * mat2[0] + mat1[7] * mat2[1] + 
            mat1[11] * mat2[2] + mat1[15] * mat2[3];
        mat[7] = mat1[3] * mat2[4] + mat1[7] * mat2[5] + 
            mat1[11] * mat2[6] + mat1[15] * mat2[7];
        mat[11] = mat1[3] * mat2[8] + mat1[7] * mat2[9] + 
            mat1[11] * mat2[10] + mat1[15] * mat2[11];
        mat[15] = mat1[3] * mat2[12] + mat1[7] * mat2[13] + 
            mat1[11] * mat2[14] + mat1[15] * mat2[15];

        // set matrix
        set(mat);
    }
}

void Matrix4x4d::multiplyFast(const Matrix4x4d& m1, const Matrix4x4d& m2)
{
    const double *mat1 = m1._data;
    const double *mat2 = m2._data;

    // 1 row
    _data[0] = mat1[0] * mat2[0] + mat1[4] * mat2[1] + 
               mat1[8] * mat2[2] + mat1[12] * mat2[3];
    _data[4] = mat1[0] * mat2[4] + mat1[4] * mat2[5] + 
               mat1[8] * mat2[6] + mat1[12] * mat2[7];
    _data[8] = mat1[0] * mat2[8] + mat1[4] * mat2[9] + 
               mat1[8] * mat2[10] + mat1[12] * mat2[11];
    _data[12] = mat1[0] * mat2[12] + mat1[4] * mat2[13] + 
                mat1[8] * mat2[14] + mat1[12] * mat2[15];

    // 2 row
    _data[1] = mat1[1] * mat2[0] + mat1[5] * mat2[1] + 
               mat1[9] * mat2[2] + mat1[13] * mat2[3];
    _data[5] = mat1[1] * mat2[4] + mat1[5] * mat2[5] + 
               mat1[9] * mat2[6] + mat1[13] * mat2[7];
    _data[9] = mat1[1] * mat2[8] + mat1[5] * mat2[9] + 
               mat1[9] * mat2[10] + mat1[13] * mat2[11];
    _data[13] = mat1[1] * mat2[12] + mat1[5] * mat2[13] + 
                mat1[9] * mat2[14] + mat1[13] * mat2[15];

    // 3 row
    _data[2] = mat1[2] * mat2[0] + mat1[6] * mat2[1] + 
               mat1[10] * mat2[2] + mat1[14] * mat2[3];
    _data[6] = mat1[2] * mat2[4] + mat1[6] * mat2[5] + 
               mat1[10] * mat2[6] + mat1[14] * mat2[7];
    _data[10] = mat1[2] * mat2[8] + mat1[6] * mat2[9] + 
                mat1[10] * mat2[10] + mat1[14] * mat2[11];
    _data[14] = mat1[2] * mat2[12] + mat1[6] * mat2[13] + 
                mat1[10] * mat2[14] + mat1[14] * mat2[15];

    // 4 row
    _data[3] = mat1[3] * mat2[0] + mat1[7] * mat2[1] + 
               mat1[11] * mat2[2] + mat1[15] * mat2[3];
    _data[7] = mat1[3] * mat2[4] + mat1[7] * mat2[5] + 
               mat1[11] * mat2[6] + mat1[15] * mat2[7];
    _data[11] = mat1[3] * mat2[8] + mat1[7] * mat2[9] + 
                mat1[11] * mat2[10] + mat1[15] * mat2[11];
    _data[15] = mat1[3] * mat2[12] + mat1[7] * mat2[13] + 
                mat1[11] * mat2[14] + mat1[15] * mat2[15];
}

void Matrix4x4d::getRotation(Rotation& rotation) 
{
    double c = (_data[0] + _data[5] + _data[10] - 1.0) * 0.5;

    rotation.setAxis(_data[6] - _data[9], _data[8] - _data[2], _data[1] - _data[4]);

    double len = sqrt(rotation.getX() * rotation.getX() + 
                      rotation.getY() * rotation.getY() +
                      rotation.getZ() * rotation.getZ());

    double s = 0.5 * len;

    rotation.setAngle(atan2(s, c));

    if (rotation.getX() == 0.0 &&
        rotation.getY() == 0.0 &&
        rotation.getZ() == 0.0) {
        rotation.set(0, 1, 0, 0);
    } else {
        len = 1.0 / len;
        rotation.setAxis(rotation.getX() * len,
                         rotation.getY() * len,
                         rotation.getZ() * len);
    }
}

void Matrix4x4d::invert()
{
    double det =
        _data[12] * _data[9] * _data[6] * _data[3]-
        _data[8] * _data[13] * _data[6] * _data[3]-
        _data[12] * _data[5] * _data[10] * _data[3]+
        _data[4] * _data[13] * _data[10] * _data[3]+
        _data[8] * _data[5] * _data[14] * _data[3]-
        _data[4] * _data[9] * _data[14] * _data[3]-
        _data[12] * _data[9] * _data[2] * _data[7]+
        _data[8] * _data[13] * _data[2] * _data[7]+
        _data[12] * _data[1] * _data[10] * _data[7]-
        _data[0] * _data[13] * _data[10] * _data[7]-
        _data[8] * _data[1] * _data[14] * _data[7]+
        _data[0] * _data[9] * _data[14] * _data[7]+
        _data[12] * _data[5] * _data[2] * _data[11]-
        _data[4] * _data[13] * _data[2] * _data[11]-
        _data[12] * _data[1] * _data[6] * _data[11]+
        _data[0] * _data[13] * _data[6] * _data[11]+
        _data[4] * _data[1] * _data[14] * _data[11]-
        _data[0] * _data[5] * _data[14] * _data[11]-
        _data[8] * _data[5] * _data[2] * _data[15]+
        _data[4] * _data[9] * _data[2] * _data[15]+
        _data[8] * _data[1] * _data[6] * _data[15]-
        _data[0] * _data[9] * _data[6] * _data[15]-
        _data[4] * _data[1] * _data[10] * _data[15]+
        _data[0] * _data[5] * _data[10] * _data[15];

    if ( det == 0.0 ) return;
    det = 1.0 / det;

    double mat[16];

    mat[0] = (_data[9]*_data[14]*_data[7] -
              _data[13]*_data[10]*_data[7] +
              _data[13]*_data[6]*_data[11] -
              _data[5]*_data[14]*_data[11] -
              _data[9]*_data[6]*_data[15] +
              _data[5]*_data[10]*_data[15]) * det;

    mat[4] = (_data[12]*_data[10]*_data[7] -
              _data[8]*_data[14]*_data[7] -
              _data[12]*_data[6]*_data[11] +
              _data[4]*_data[14]*_data[11] +
              _data[8]*_data[6]*_data[15] -
              _data[4]*_data[10]*_data[15]) * det;

    mat[8] = (_data[8]*_data[13]*_data[7] -
              _data[12]*_data[9]*_data[7] +
              _data[12]*_data[5]*_data[11] -
              _data[4]*_data[13]*_data[11] -
              _data[8]*_data[5]*_data[15] +
              _data[4]*_data[9]*_data[15]) * det;

    mat[12] = (_data[12]*_data[9]*_data[6] -
               _data[8]*_data[13]*_data[6] -
               _data[12]*_data[5]*_data[10] +
               _data[4]*_data[13]*_data[10] +
               _data[8]*_data[5]*_data[14] -
               _data[4]*_data[9]*_data[14]) * det;

    mat[1] = (_data[13]*_data[10]*_data[3] -
              _data[9]*_data[14]*_data[3] -
              _data[13]*_data[2]*_data[11] +
              _data[1]*_data[14]*_data[11] +
              _data[9]*_data[2]*_data[15] -
              _data[1]*_data[10]*_data[15]) * det;

    mat[5] = (_data[8]*_data[14]*_data[3] -
              _data[12]*_data[10]*_data[3] +
              _data[12]*_data[2]*_data[11] -
              _data[0]*_data[14]*_data[11] -
              _data[8]*_data[2]*_data[15] +
              _data[0]*_data[10]*_data[15]) * det;

    mat[9] = (_data[12]*_data[9]*_data[3] -
              _data[8]*_data[13]*_data[3] -
              _data[12]*_data[1]*_data[11] +
              _data[0]*_data[13]*_data[11] +
              _data[8]*_data[1]*_data[15] -
              _data[0]*_data[9]*_data[15]) * det;

    mat[13] = (_data[8]*_data[13]*_data[2] -
               _data[12]*_data[9]*_data[2] +
               _data[12]*_data[1]*_data[10] -
               _data[0]*_data[13]*_data[10] -
               _data[8]*_data[1]*_data[14] +
               _data[0]*_data[9]*_data[14]) * det;

    mat[2] = (_data[5]*_data[14]*_data[3] -
              _data[13]*_data[6]*_data[3] +
              _data[13]*_data[2]*_data[7] -
              _data[1]*_data[14]*_data[7] -
              _data[5]*_data[2]*_data[15] +
              _data[1]*_data[6]*_data[15]) * det;

    mat[6] = (_data[12]*_data[6]*_data[3] -
              _data[4]*_data[14]*_data[3] -
              _data[12]*_data[2]*_data[7] +
              _data[0]*_data[14]*_data[7] +
              _data[4]*_data[2]*_data[15] -
              _data[0]*_data[6]*_data[15]) * det;

    mat[10] = (_data[4]*_data[13]*_data[3] -
               _data[12]*_data[5]*_data[3] +
               _data[12]*_data[1]*_data[7] -
               _data[0]*_data[13]*_data[7] -
               _data[4]*_data[1]*_data[15] +
               _data[0]*_data[5]*_data[15]) * det;

    mat[14] = (_data[12]*_data[5]*_data[2] -
               _data[4]*_data[13]*_data[2] -
               _data[12]*_data[1]*_data[6] +
               _data[0]*_data[13]*_data[6] +
               _data[4]*_data[1]*_data[14] -
               _data[0]*_data[5]*_data[14]) * det;

    mat[3] = (_data[9]*_data[6]*_data[3] -
              _data[5]*_data[10]*_data[3] -
              _data[9]*_data[2]*_data[7] +
              _data[1]*_data[10]*_data[7] +
              _data[5]*_data[2]*_data[11] -
              _data[1]*_data[6]*_data[11]) * det;

    mat[7] = (_data[4]*_data[10]*_data[3] -
              _data[8]*_data[6]*_data[3] +
              _data[8]*_data[2]*_data[7] -
              _data[0]*_data[10]*_data[7] -
              _data[4]*_data[2]*_data[11] +
              _data[0]*_data[6]*_data[11]) * det;

    mat[11] = (_data[8]*_data[5]*_data[3] -
               _data[4]*_data[9]*_data[3] -
               _data[8]*_data[1]*_data[7] +
               _data[0]*_data[9]*_data[7] +
               _data[4]*_data[1]*_data[11] -
               _data[0]*_data[5]*_data[11]) * det;

    mat[15] = (_data[4]*_data[9]*_data[2] -
               _data[8]*_data[5]*_data[2] +
               _data[8]*_data[1]*_data[6] -
               _data[0]*_data[9]*_data[6] -
               _data[4]*_data[1]*_data[10] +
               _data[0]*_data[5]*_data[10]) * det;

    set(mat);
}

void Matrix4x4d::invert(const Matrix4x4d& mat)
{
    if (&mat == this) {
        invert();
    } else {
        invertFast(mat);
    }
}

void Matrix4x4d::invertFast(const Matrix4x4d& mat)
{
    const double *srcData = mat._data;

    double det =
        srcData[12] * srcData[9] * srcData[6] * srcData[3]-
        srcData[8] * srcData[13] * srcData[6] * srcData[3]-
        srcData[12] * srcData[5] * srcData[10] * srcData[3]+
        srcData[4] * srcData[13] * srcData[10] * srcData[3]+
        srcData[8] * srcData[5] * srcData[14] * srcData[3]-
        srcData[4] * srcData[9] * srcData[14] * srcData[3]-
        srcData[12] * srcData[9] * srcData[2] * srcData[7]+
        srcData[8] * srcData[13] * srcData[2] * srcData[7]+
        srcData[12] * srcData[1] * srcData[10] * srcData[7]-
        srcData[0] * srcData[13] * srcData[10] * srcData[7]-
        srcData[8] * srcData[1] * srcData[14] * srcData[7]+
        srcData[0] * srcData[9] * srcData[14] * srcData[7]+
        srcData[12] * srcData[5] * srcData[2] * srcData[11]-
        srcData[4] * srcData[13] * srcData[2] * srcData[11]-
        srcData[12] * srcData[1] * srcData[6] * srcData[11]+
        srcData[0] * srcData[13] * srcData[6] * srcData[11]+
        srcData[4] * srcData[1] * srcData[14] * srcData[11]-
        srcData[0] * srcData[5] * srcData[14] * srcData[11]-
        srcData[8] * srcData[5] * srcData[2] * srcData[15]+
        srcData[4] * srcData[9] * srcData[2] * srcData[15]+
        srcData[8] * srcData[1] * srcData[6] * srcData[15]-
        srcData[0] * srcData[9] * srcData[6] * srcData[15]-
        srcData[4] * srcData[1] * srcData[10] * srcData[15]+
        srcData[0] * srcData[5] * srcData[10] * srcData[15];

    if ( det == 0.0 ) return;
    det = 1.0 / det;

    _data[0] = (srcData[9]*srcData[14]*srcData[7] -
                srcData[13]*srcData[10]*srcData[7] +
                srcData[13]*srcData[6]*srcData[11] -
                srcData[5]*srcData[14]*srcData[11] -
                srcData[9]*srcData[6]*srcData[15] +
                srcData[5]*srcData[10]*srcData[15]) * det;

    _data[4] = (srcData[12]*srcData[10]*srcData[7] -
                srcData[8]*srcData[14]*srcData[7] -
                srcData[12]*srcData[6]*srcData[11] +
                srcData[4]*srcData[14]*srcData[11] +
                srcData[8]*srcData[6]*srcData[15] -
                srcData[4]*srcData[10]*srcData[15]) * det;

    _data[8] = (srcData[8]*srcData[13]*srcData[7] -
                srcData[12]*srcData[9]*srcData[7] +
                srcData[12]*srcData[5]*srcData[11] -
                srcData[4]*srcData[13]*srcData[11] -
                srcData[8]*srcData[5]*srcData[15] +
                srcData[4]*srcData[9]*srcData[15]) * det;

    _data[12] = (srcData[12]*srcData[9]*srcData[6] -
                 srcData[8]*srcData[13]*srcData[6] -
                 srcData[12]*srcData[5]*srcData[10] +
                 srcData[4]*srcData[13]*srcData[10] +
                 srcData[8]*srcData[5]*srcData[14] -
                 srcData[4]*srcData[9]*srcData[14]) * det;

    _data[1] = (srcData[13]*srcData[10]*srcData[3] -
                srcData[9]*srcData[14]*srcData[3] -
                srcData[13]*srcData[2]*srcData[11] +
                srcData[1]*srcData[14]*srcData[11] +
                srcData[9]*srcData[2]*srcData[15] -
                srcData[1]*srcData[10]*srcData[15]) * det;

    _data[5] = (srcData[8]*srcData[14]*srcData[3] -
                srcData[12]*srcData[10]*srcData[3] +
                srcData[12]*srcData[2]*srcData[11] -
                srcData[0]*srcData[14]*srcData[11] -
                srcData[8]*srcData[2]*srcData[15] +
                srcData[0]*srcData[10]*srcData[15]) * det;

    _data[9] = (srcData[12]*srcData[9]*srcData[3] -
                srcData[8]*srcData[13]*srcData[3] -
                srcData[12]*srcData[1]*srcData[11] +
                srcData[0]*srcData[13]*srcData[11] +
                srcData[8]*srcData[1]*srcData[15] -
                srcData[0]*srcData[9]*srcData[15]) * det;

    _data[13] = (srcData[8]*srcData[13]*srcData[2] -
                 srcData[12]*srcData[9]*srcData[2] +
                 srcData[12]*srcData[1]*srcData[10] -
                 srcData[0]*srcData[13]*srcData[10] -
                 srcData[8]*srcData[1]*srcData[14] +
                 srcData[0]*srcData[9]*srcData[14]) * det;

    _data[2] = (srcData[5]*srcData[14]*srcData[3] -
                srcData[13]*srcData[6]*srcData[3] +
                srcData[13]*srcData[2]*srcData[7] -
                srcData[1]*srcData[14]*srcData[7] -
                srcData[5]*srcData[2]*srcData[15] +
                srcData[1]*srcData[6]*srcData[15]) * det;

    _data[6] = (srcData[12]*srcData[6]*srcData[3] -
                srcData[4]*srcData[14]*srcData[3] -
                srcData[12]*srcData[2]*srcData[7] +
                srcData[0]*srcData[14]*srcData[7] +
                srcData[4]*srcData[2]*srcData[15] -
                srcData[0]*srcData[6]*srcData[15]) * det;

    _data[10] = (srcData[4]*srcData[13]*srcData[3] -
                 srcData[12]*srcData[5]*srcData[3] +
                 srcData[12]*srcData[1]*srcData[7] -
                 srcData[0]*srcData[13]*srcData[7] -
                 srcData[4]*srcData[1]*srcData[15] +
                 srcData[0]*srcData[5]*srcData[15]) * det;

    _data[14] = (srcData[12]*srcData[5]*srcData[2] -
                 srcData[4]*srcData[13]*srcData[2] -
                 srcData[12]*srcData[1]*srcData[6] +
                 srcData[0]*srcData[13]*srcData[6] +
                 srcData[4]*srcData[1]*srcData[14] -
                 srcData[0]*srcData[5]*srcData[14]) * det;

    _data[3] = (srcData[9]*srcData[6]*srcData[3] -
                srcData[5]*srcData[10]*srcData[3] -
                srcData[9]*srcData[2]*srcData[7] +
                srcData[1]*srcData[10]*srcData[7] +
                srcData[5]*srcData[2]*srcData[11] -
                srcData[1]*srcData[6]*srcData[11]) * det;

    _data[7] = (srcData[4]*srcData[10]*srcData[3] -
                srcData[8]*srcData[6]*srcData[3] +
                srcData[8]*srcData[2]*srcData[7] -
                srcData[0]*srcData[10]*srcData[7] -
                srcData[4]*srcData[2]*srcData[11] +
                srcData[0]*srcData[6]*srcData[11]) * det;

    _data[11] = (srcData[8]*srcData[5]*srcData[3] -
                 srcData[4]*srcData[9]*srcData[3] -
                 srcData[8]*srcData[1]*srcData[7] +
                 srcData[0]*srcData[9]*srcData[7] +
                 srcData[4]*srcData[1]*srcData[11] -
                 srcData[0]*srcData[5]*srcData[11]) * det;

    _data[15] = (srcData[4]*srcData[9]*srcData[2] -
                 srcData[8]*srcData[5]*srcData[2] +
                 srcData[8]*srcData[1]*srcData[6] -
                 srcData[0]*srcData[9]*srcData[6] -
                 srcData[4]*srcData[1]*srcData[10] +
                 srcData[0]*srcData[5]*srcData[10]) * det;
}

Matrix4x4d
Matrix4x4d::inverse() const
{
    const double *m = _data;
    Matrix4x4d p;

    double m00 = m[0], m01 = m[4], m02 = m[8], m03 = m[12];
    double m10 = m[1], m11 = m[5], m12 = m[9], m13 = m[13];
    double m20 = m[2], m21 = m[6], m22 = m[10], m23 = m[14];
    double m30 = m[3], m31 = m[7], m32 = m[11], m33 = m[15];

    double det0, inv0;

#define det33(a1,a2,a3,b1,b2,b3,c1,c2,c3) \
    (a1*(b2*c3-b3*c2) + b1*(c2*a3-a2*c3) + c1*(a2*b3-a3*b2))

    double cof00 =  det33(m11, m12, m13, m21, m22, m23, m31, m32, m33);
    double cof01 = -det33(m12, m13, m10, m22, m23, m20, m32, m33, m30);
    double cof02 =  det33(m13, m10, m11, m23, m20, m21, m33, m30, m31);
    double cof03 = -det33(m10, m11, m12, m20, m21, m22, m30, m31, m32);

    double cof10 = -det33(m21, m22, m23, m31, m32, m33, m01, m02, m03);
    double cof11 =  det33(m22, m23, m20, m32, m33, m30, m02, m03, m00);
    double cof12 = -det33(m23, m20, m21, m33, m30, m31, m03, m00, m01);
    double cof13 =  det33(m20, m21, m22, m30, m31, m32, m00, m01, m02);

    double cof20 =  det33(m31, m32, m33, m01, m02, m03, m11, m12, m13);
    double cof21 = -det33(m32, m33, m30, m02, m03, m00, m12, m13, m10);
    double cof22 =  det33(m33, m30, m31, m03, m00, m01, m13, m10, m11);
    double cof23 = -det33(m30, m31, m32, m00, m01, m02, m10, m11, m12);

    double cof30 = -det33(m01, m02, m03, m11, m12, m13, m21, m22, m23);
    double cof31 =  det33(m02, m03, m00, m12, m13, m10, m22, m23, m20);
    double cof32 = -det33(m03, m00, m01, m13, m10, m11, m23, m20, m21);
    double cof33 =  det33(m00, m01, m02, m10, m11, m12, m20, m21, m22);

#undef det33

#if 0
    TRACE("Invert:");
    TRACE("   %12.9f %12.9f %12.9f %12.9f", m00, m01, m02, m03);
    TRACE("   %12.9f %12.9f %12.9f %12.9f", m10, m11, m12, m13);
    TRACE("   %12.9f %12.9f %12.9f %12.9f", m20, m21, m22, m23);
    TRACE("   %12.9f %12.9f %12.9f %12.9f", m30, m31, m32, m33);
#endif

#if 0
    det0  = m00 * cof00 + m01 * cof01 + m02 * cof02 + m03 * cof03;
#else
    det0  = m30 * cof30 + m31 * cof31 + m32 * cof32 + m33 * cof33;
#endif
    inv0  = (det0 == 0.0f) ? 0.0f : 1.0f / det0;

    p.get()[0] = cof00 * inv0;
    p.get()[1] = cof01 * inv0;
    p.get()[2] = cof02 * inv0;
    p.get()[3] = cof03 * inv0;

    p.get()[4] = cof10 * inv0;
    p.get()[5] = cof11 * inv0;
    p.get()[6] = cof12 * inv0;
    p.get()[7] = cof13 * inv0;

    p.get()[8] = cof20 * inv0;
    p.get()[9] = cof21 * inv0;
    p.get()[10] = cof22 * inv0;
    p.get()[11] = cof23 * inv0;

    p.get()[12] = cof30 * inv0;
    p.get()[13] = cof31 * inv0;
    p.get()[14] = cof32 * inv0;
    p.get()[15] = cof33 * inv0;	

    return p;
}

void Matrix4x4d::transpose()
{
    double m[16];

    m[0] = _data[0];
    m[1] = _data[4];
    m[2] = _data[8];
    m[3] = _data[12];

    m[4] = _data[1];
    m[5] = _data[5];
    m[6] = _data[9];
    m[7] = _data[13];

    m[8] = _data[2];
    m[9] = _data[6];
    m[10] = _data[10];
    m[11] = _data[14];

    m[12] = _data[3];
    m[13] = _data[7];
    m[14] = _data[11];
    m[15] = _data[15];

    set(m);
}

void Matrix4x4d::transposeFast(const Matrix4x4d& mat)
{
    _data[0] = mat._data[0];
    _data[1] = mat._data[4];
    _data[2] = mat._data[8];
    _data[3] = mat._data[12];

    _data[4] = mat._data[1];
    _data[5] = mat._data[5];
    _data[6] = mat._data[9];
    _data[7] = mat._data[13];

    _data[8] = mat._data[2];
    _data[9] = mat._data[6];
    _data[10] = mat._data[10];
    _data[11] = mat._data[14];

    _data[12] = mat._data[3];
    _data[13] = mat._data[7];
    _data[14] = mat._data[11];
    _data[15] = mat._data[15];
}

void Matrix4x4d::transpose(const Matrix4x4d& mat)
{
    if (&mat == this) {
        transpose();
    } else {
        transposeFast(mat);
    }
}

void Matrix4x4d::setFloat(const float *m)
{
    for (int i = 0; i < 16; ++i) {
        _data[i] = m[i];
    }
}

void Matrix4x4d::multiply(const Matrix4x4d& mat1, const Vector3f& position)
{
    Matrix4x4d mat;
    mat.makeTranslation(position);
    multiply(mat1, mat);
}

void Matrix4x4d::multiply(const Matrix4x4d& mat1, const Rotation& rotation)
{
    Matrix4x4d mat;
    mat.makeRotation(rotation);
    multiply(mat1, mat);
}

void Matrix4x4d::multiply(const Vector3f& position, const Matrix4x4d& mat1)
{
    Matrix4x4d mat;
    mat.makeTranslation(position);
    multiply(mat, mat1);
}

void Matrix4x4d::multiply(const Rotation& rotation, const Matrix4x4d& mat1)
{
    Matrix4x4d mat;
    mat.makeRotation(rotation);
    multiply(mat, mat1);
}

void Matrix4x4d::makeVecRotVec(const Vector3f& vec1, const Vector3f& vec2)
{
    Vector3f axis = cross(vec1, vec2);

    float angle = atan2(axis.length(), vec1.dot(vec2));
    axis = axis.normalize();
    makeRotation(axis.x, axis.y, axis.z, angle);
}

void Matrix4x4d::multiplyScale(const Matrix4x4d& mat1, const Vector3f& scale)
{
    Matrix4x4d mat;
    mat.makeScale(scale);
    multiply(mat1, mat);
}

void Matrix4x4d::multiplyScale(const Vector3f& scale, const Matrix4x4d& mat1)
{
    Matrix4x4d mat;
    mat.makeScale(scale);
    multiply(mat, mat1);
}