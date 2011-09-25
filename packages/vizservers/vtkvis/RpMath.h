/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_MATH_H__
#define __RAPPTURE_VTKVIS_MATH_H__

#include <cmath>
#include <cstring>

#include <vtkMath.h>
#include <vtkMatrix4x4.h>

namespace Rappture {
namespace VtkVis {

/**
 * \brief Convert a quaternion to an axis/angle rotation
 * 
 * \param[in] quat Quaternion with scalar first: wxyz
 * \param[out] angleAxis axis/angle rotation with angle in degrees first
 */
inline void quatToAngleAxis(const double quat[4], double angleAxis[4])
{
    angleAxis[0] = vtkMath::DegreesFromRadians(2.0 * acos(quat[0]));
    if (angleAxis[0] < 1.0e-6) {
        angleAxis[1] = 1;
        angleAxis[2] = 0;
        angleAxis[3] = 0;
    } else {
        double denom = sqrt(1. - quat[0] * quat[0]);
        angleAxis[1] = quat[1] / denom;
        angleAxis[2] = quat[2] / denom;
        angleAxis[3] = quat[3] / denom;
    }
}

/**
 * \brief Convert an axis/angle rotation to a quaternion
 * 
 * \param[in] angleAxis axis/angle rotation with angle in degrees first
 * \param[out] quat Quaternion with scalar first: wxyz
 */
inline void angleAxisToQuat(const double angleAxis[4], double quat[4])
{
    quat[0] = cos(vtkMath::RadiansFromDegrees(angleAxis[0]) / 2.0);
    double sinHalfAngle = sin(vtkMath::RadiansFromDegrees(angleAxis[0]) / 2.0);
    quat[1] = angleAxis[1] * sinHalfAngle;
    quat[2] = angleAxis[2] * sinHalfAngle;
    quat[3] = angleAxis[3] * sinHalfAngle;
}

/**
 * \brief Fill a vtkMatrix4x4 from a quaternion
 *
 * \param[in] quat Quaternion with scalar first: wxyz
 * \param[out] mat Matrix to be filled in
 */
inline void quaternionToMatrix4x4(const double quat[4], vtkMatrix4x4& mat)
{
    double ww = quat[0]*quat[0];
    double wx = quat[0]*quat[1];
    double wy = quat[0]*quat[2];
    double wz = quat[0]*quat[3];

    double xx = quat[1]*quat[1];
    double yy = quat[2]*quat[2];
    double zz = quat[3]*quat[3];

    double xy = quat[1]*quat[2];
    double xz = quat[1]*quat[3];
    double yz = quat[2]*quat[3];

    double rr = xx + yy + zz;
    // normalization factor, just in case quaternion was not normalized
    double f = double(1)/double(sqrt(ww + rr));
    double s = (ww - rr)*f;
    f *= 2;

    mat[0][0] = xx*f + s;
    mat[1][0] = (xy + wz)*f;
    mat[2][0] = (xz - wy)*f;

    mat[0][1] = (xy - wz)*f;
    mat[1][1] = yy*f + s;
    mat[2][1] = (yz + wx)*f;

    mat[0][2] = (xz + wy)*f;
    mat[1][2] = (yz - wx)*f;
    mat[2][2] = zz*f + s;
}

/**
 * \brief Fill a vtkMatrix4x4 from a quaternion, but with the matrix
 * transposed.
 *
 * \param[in] quat Quaternion with scalar first: wxyz
 * \param[out] mat Matrix to be filled in
 */
inline void quaternionToTransposeMatrix4x4(const double quat[4], vtkMatrix4x4& mat)
{
    double ww = quat[0]*quat[0];
    double wx = quat[0]*quat[1];
    double wy = quat[0]*quat[2];
    double wz = quat[0]*quat[3];

    double xx = quat[1]*quat[1];
    double yy = quat[2]*quat[2];
    double zz = quat[3]*quat[3];

    double xy = quat[1]*quat[2];
    double xz = quat[1]*quat[3];
    double yz = quat[2]*quat[3];

    double rr = xx + yy + zz;
    // normalization factor, just in case quaternion was not normalized
    double f = double(1)/double(sqrt(ww + rr));
    double s = (ww - rr)*f;
    f *= 2;

    mat[0][0] = xx*f + s;
    mat[0][1] = (xy + wz)*f;
    mat[0][2] = (xz - wy)*f;

    mat[1][0] = (xy - wz)*f;
    mat[1][1] = yy*f + s;
    mat[1][2] = (yz + wx)*f;

    mat[2][0] = (xz + wy)*f;
    mat[2][1] = (yz - wx)*f;
    mat[2][2] = zz*f + s;
}

/**
 * \brief Multiply two quaternions and return the result
 *
 * Note: result can be the same memory location as one of the
 * inputs
 */
inline double *quatMult(const double q1[4], const double q2[4], double result[4])
{
    double q1w = q1[0];
    double q1x = q1[1];
    double q1y = q1[2];
    double q1z = q1[3];
    double q2w = q2[0];
    double q2x = q2[1];
    double q2y = q2[2];
    double q2z = q2[3];
    result[0] = (q1w*q2w) - (q1x*q2x) - (q1y*q2y) - (q1z*q2z);
    result[1] = (q1w*q2x) + (q1x*q2w) + (q1y*q2z) - (q1z*q2y);
    result[2] = (q1w*q2y) + (q1y*q2w) + (q1z*q2x) - (q1x*q2z);
    result[3] = (q1w*q2z) + (q1z*q2w) + (q1x*q2y) - (q1y*q2x);
    return result;
}

/**
 * \brief Compute the conjugate of a quaternion
 *
 * Note: result can be the same memory location as the input
 */
inline double *quatConjugate(const double quat[4], double result[4])
{
    if (result != quat)
        result[0] = quat[0];
    result[1] = -quat[1];
    result[2] = -quat[2];
    result[3] = -quat[3];
    return result;
}

/**
 * \brief Compute the reciprocal of a quaternion
 *
 * Note: result can be the same memory location as the input
 */
inline double *quatReciprocal(const double quat[4], double result[4])
{
    double denom = 
        quat[0]*quat[0] + 
        quat[1]*quat[1] + 
        quat[2]*quat[2] + 
        quat[3]*quat[3];
    Rappture::VtkVis::quatConjugate(quat, result);
    for (int i = 0; i < 4; i++) {
        result[i] /= denom;
    }
    return result;
}

/**
 * \brief Compute the reciprocal of a quaternion in place
 */
inline double *quatReciprocal(double quat[4])
{
    return Rappture::VtkVis::quatReciprocal(quat, quat);
}

/**
 * \brief Deep copy a quaternion
 */
inline void copyQuat(const double quat[4], double result[4])
{
    memcpy(result, quat, sizeof(double)*4);
}

}
}

#endif
