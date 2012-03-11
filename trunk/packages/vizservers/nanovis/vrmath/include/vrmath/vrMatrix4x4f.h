/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRMATRIX4X4_H
#define VRMATRIX4X4_H

#include <vrmath/vrLinmath.h>
#include <vrmath/vrVector3f.h>
#include <vrmath/vrRotation.h>
#include <memory.h>

class vrMatrix4x4f
{
public:
    /**
     * @brief make a identity matrix
     */
    void makeIdentity();

    /**
     * @brief make a translation matrix
     * @param translation translation
     */
    void makeTranslation(const vrVector3f& translation);

    /**
     * @brief make a translation matrix
     * @param x x position
     */
    void makeTranslation(float x, float y, float z);

    /**
     * @brief make a rotation matrix
     * @param rotation rotation
     */
    void makeRotation(const vrRotation& rotation);

    /**
     * @brief Make a rotation matrix
     * @param rotation rotation
     */
    void makeRotation(float x, float y, float z, float angle);

    void makeVecRotVec(const vrVector3f& vec1, const vrVector3f& vec2);

    /**
     * @brief make a scale matrix
     * @param scale scale
     */
    void makeScale(const vrVector3f& scale);
    void makeTR(const vrVector3f& translation, const vrRotation& rotation);
    void makeTRS(const vrVector3f& translation, const vrRotation& rotation, const vrVector3f& scale);

    void multiply(const vrMatrix4x4f& mat1, const vrMatrix4x4f& mat2);

    void multiply(const vrMatrix4x4f& mat1, const vrVector3f& position);
    void multiplyScale(const vrMatrix4x4f& mat, const vrVector3f& scale);
    void multiply(const vrMatrix4x4f& mat1, const vrRotation& rotation);
    void multiply(const vrVector3f& position, const vrMatrix4x4f& mat1);
    void multiply(const vrRotation& rotation, const vrMatrix4x4f& mat1);
    void multiplyScale(const vrVector3f& scale, const vrMatrix4x4f& mat);

    void multiply(const vrMatrix4x4f& mat1);
    void multiplyFast(const vrMatrix4x4f& mat1, const vrMatrix4x4f& mat2);

    void invert();
    void invert(const vrMatrix4x4f& mat);
    void invertFast(const vrMatrix4x4f& mat);

    void transpose();
    void transpose(const vrMatrix4x4f& mat);
    void transposeFast(const vrMatrix4x4f& mat);
	
    void getTranslation(vrVector3f& vector);
    void getRotation(vrRotation& rotation);

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
    void set(float *m);

    /**
     * @brief set double arrary to this
     * @param m float matrix values
     */
    void set(double *m);

private:
    float _data[16];
};

inline const float *vrMatrix4x4f::get() const
{
    return _data;
}

inline float *vrMatrix4x4f::get()
{
    return _data;
}

inline void vrMatrix4x4f::set(float *m)
{
    memcpy(_data, m, sizeof(float) * 16);
}


inline void vrMatrix4x4f::getTranslation(vrVector3f& translation)
{
    translation.set(_data[12],_data[13], _data[14]);
}

inline void vrMatrix4x4f::makeTRS(const vrVector3f& translation, const vrRotation& rotation, const vrVector3f& scale)
{
    vrMatrix4x4f mat;
    mat.makeTR(translation, rotation);
	
    makeScale(scale);
	
    multiply(mat, *this);
}

inline void vrMatrix4x4f::makeTR(const vrVector3f& translation, const vrRotation& rotation)
{
    makeRotation(rotation);
	
    _data[12] = translation.x;
    _data[13] = translation.y;
    _data[14] = translation.z;
}

#endif
