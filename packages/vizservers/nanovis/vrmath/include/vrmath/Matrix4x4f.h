/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */
#ifndef VRMATRIX4X4F_H
#define VRMATRIX4X4F_H

#include <memory.h>

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>

#include "Trace.h"

namespace vrmath {

class Rotation;

class Matrix4x4f
{
public:
    Matrix4x4f()
    {
        makeIdentity();
    }

    Matrix4x4f(float *m)
    {
        set(m);
    }

    Matrix4x4f(const Matrix4x4f& other)
    {
        set(other.get());
    }

    Matrix4x4f& operator=(const Matrix4x4f& other)
    {
        if (&other != this) {
            set(other.get());
        }
        return *this;
    }

    /**
     * @brief make a identity matrix
     */
    void makeIdentity();

    /**
     * @brief make a translation matrix
     */
    void makeTranslation(const Vector3f& translation);

    /**
     * @brief make a translation matrix
     */
    void makeTranslation(float x, float y, float z);

    /**
     * @brief make a rotation matrix
     */
    void makeRotation(const Rotation& rotation);

    /**
     * @brief Make a rotation matrix
     */
    void makeRotation(double x, double y, double z, double angle);

    void makeVecRotVec(const Vector3f& vec1, const Vector3f& vec2);

    /**
     * @brief make a scale matrix
     */
    void makeScale(const Vector3f& scale);

    void makeScale(float x, float y, float z);

    void makeTR(const Vector3f& translation, const Rotation& rotation);
    void makeTRS(const Vector3f& translation, const Rotation& rotation, const Vector3f& scale);

    Vector4f transform(const Vector4f& v) const;
    Vector3f transformVec(const Vector3f& v) const;

    Vector4f preMultiplyRowVector(const Vector4f& v) const;

    void multiply(const Matrix4x4f& mat1, const Matrix4x4f& mat2);

    void multiply(const Matrix4x4f& mat1, const Vector3f& position);
    void multiplyScale(const Matrix4x4f& mat, const Vector3f& scale);
    void multiply(const Matrix4x4f& mat1, const Rotation& rotation);
    void multiply(const Vector3f& position, const Matrix4x4f& mat1);
    void multiply(const Rotation& rotation, const Matrix4x4f& mat1);
    void multiplyScale(const Vector3f& scale, const Matrix4x4f& mat);

    void multiply(const Matrix4x4f& mat1);
    void multiplyFast(const Matrix4x4f& mat1, const Matrix4x4f& mat2);

    void invert();
    void invert(const Matrix4x4f& mat);
    void invertFast(const Matrix4x4f& mat);

    void transpose();
    void transpose(const Matrix4x4f& mat);
    void transposeFast(const Matrix4x4f& mat);
	
    void getTranslation(Vector3f& vector);
    void getRotation(Rotation& rotation);

    /**
     * @brief return data pointer of the matrix
     * @return float array of the matrix
     */
    const float *get() const;

    /**
     * @brief return data pointer of the matrix
     * @return float array of the matrix
     */
    float *get();

    /**
     * @brief set float arrary to this
     * @param m float matrix values
     */
    void set(const float *m);

    /**
     * @brief set double arrary to this
     * @param m float matrix values
     */
    void setDouble(const double *m);

    void print() const;

private:
    /**
     * \brief Column-major array (like OpenGL)
     */
    float _data[16];
};

inline const float *Matrix4x4f::get() const
{
    return _data;
}

inline float *Matrix4x4f::get()
{
    return _data;
}

inline void Matrix4x4f::set(const float *m)
{
    memcpy(_data, m, sizeof(float) * 16);
}

inline void Matrix4x4f::getTranslation(Vector3f& translation)
{
    translation.set(_data[12], _data[13], _data[14]);
}

inline void Matrix4x4f::makeTRS(const Vector3f& translation,
                                const Rotation& rotation,
                                const Vector3f& scale)
{
    Matrix4x4f mat;
    mat.makeTR(translation, rotation);
	
    makeScale(scale);
	
    multiply(mat, *this);
}

inline void Matrix4x4f::makeTR(const Vector3f& translation,
                               const Rotation& rotation)
{
    makeRotation(rotation);
	
    _data[12] = translation.x;
    _data[13] = translation.y;
    _data[14] = translation.z;
}

inline void Matrix4x4f::print() const
{
    TRACE("% 8.6f % 8.6f % 8.6f % 8.6f", _data[0], _data[4], _data[8 ], _data[12]);
    TRACE("% 8.6f % 8.6f % 8.6f % 8.6f", _data[1], _data[5], _data[9 ], _data[13]);
    TRACE("% 8.6f % 8.6f % 8.6f % 8.6f", _data[2], _data[6], _data[10], _data[14]);
    TRACE("% 8.6f % 8.6f % 8.6f % 8.6f", _data[3], _data[7], _data[11], _data[15]);
}

}
#endif
