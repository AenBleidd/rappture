/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vrmath/vrVector3f.h>
#include <vrmath/vrRotation.h>
#include <vrmath/vrMatrix4x4f.h>

void vrMatrix4x4f::makeIdentity()
{
    _data[1] = _data[2] = _data[3] = _data[4] =
	_data[6] = _data[7] = _data[8] = _data[9] =
	_data[11] = _data[12] = _data[13] = _data[14] = 0.0f;
    _data[0] = _data[5] = _data[10] = _data[15] = 1.0f;
}

void vrMatrix4x4f::makeTranslation(const vrVector3f& translation)
{
    _data[1] = _data[2] = _data[3] = _data[4] =
	_data[6] = _data[7] = _data[8] = _data[9] =
	_data[11] = 0.0f;
    _data[0] = _data[5] = _data[10] = _data[15] = 1.0f;
    _data[12] = translation.x;
    _data[13] = translation.y;
    _data[14] = translation.z;
}

void vrMatrix4x4f::makeTranslation(float x, float y, float z)
{
    _data[1] = _data[2] = _data[3] = _data[4] =
	_data[6] = _data[7] = _data[8] = _data[9] =
	_data[11] = 0.0f;
    _data[0] = _data[5] = _data[10] = _data[15] = 1.0f;
    _data[12] = x;
    _data[13] = y;
    _data[14] = z;
}

void vrMatrix4x4f::makeRotation(const vrRotation& rotation)
{
    if (rotation.getX() == 0.0f && rotation.getY() == 0.0f && rotation.getZ() == 0.0f) {
        makeIdentity();
        return;
    }

    float xAxis = rotation.getX(), yAxis = rotation.getY(), zAxis = rotation.getZ();
    float invLen = 1.0f / sqrt( xAxis * xAxis + yAxis * yAxis + zAxis * zAxis);
    float cosine = cos(rotation.getAngle());
    float sine = sin(rotation.getAngle());

    xAxis *= invLen;
    yAxis *= invLen;
    zAxis *= invLen;

    float oneMinusCosine = 1.0f - cosine;

    _data[3] = _data[7] = _data[11] = _data[12] = _data[13] = _data[14] = 0.0f;
    _data[15] = 1.0f;

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

void vrMatrix4x4f::makeRotation(float xAxis, float yAxis, float zAxis, float angle)
{
    if (xAxis == 0.0f && yAxis == 0.0f && zAxis == 0.0f) {
        makeIdentity();
        return;
    }

    float invLen = 1.0f / sqrt( xAxis * xAxis + yAxis * yAxis + zAxis * zAxis);
    float cosine = cos(angle);
    float sine = sin(angle);

    xAxis *= invLen;
    yAxis *= invLen;
    zAxis *= invLen;

    float oneMinusCosine = 1.0f - cosine;

    _data[3] = _data[7] = _data[11] = _data[12] = _data[13] = _data[14] = 0.0f;
    _data[15] = 1.0f;

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


void vrMatrix4x4f::makeScale(const vrVector3f& scale)
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

void vrMatrix4x4f::multiply(const vrMatrix4x4f& m1)
{
    float mat[16];
    float* mat1 = (float*) m1._data;

    // 1 row
    mat[0] = float(_data[0] * mat1[0] + _data[4] * mat1[1] + 
                   _data[8] * mat1[2] + _data[12] * mat1[3]);
    mat[4] = float(_data[0] * mat1[4] + _data[4] * mat1[5] + 
                   _data[8] * mat1[6] + _data[12] * mat1[7]);
    mat[8] = float(_data[0] * mat1[8] + _data[4] * mat1[9] + 
                   _data[8] * mat1[10] + _data[12] * mat1[11]);
    mat[12] = float(_data[0] * mat1[12] + _data[4] * mat1[13] + 
                    _data[8] * mat1[14] + _data[12] * mat1[15]);

    // 2 row
    mat[1] = float(_data[1] * mat1[0] + _data[5] * mat1[1] + 
                   _data[9] * mat1[2] + _data[13] * mat1[3]);
    mat[5] = float(_data[1] * mat1[4] + _data[5] * mat1[5] + 
                   _data[9] * mat1[6] + _data[13] * mat1[7]);
    mat[9] = float(_data[1] * mat1[8] + _data[5] * mat1[9] + 
                   _data[9] * mat1[10] + _data[13] * mat1[11]);
    mat[13] = float(_data[1] * mat1[12] + _data[5] * mat1[13] + 
                    _data[9] * mat1[14] + _data[13] * mat1[15]);

    // 3 row
    mat[2] = float(_data[2] * mat1[0] + _data[6] * mat1[1] + 
                   _data[10] * mat1[2] + _data[14] * mat1[3]);

    mat[6] = float(_data[2] * mat1[4] + _data[6] * mat1[5] + 
                   _data[10] * mat1[6] + _data[14] * mat1[7]);
    mat[10] = float(_data[2] * mat1[8] + _data[6] * mat1[9] + 
                    _data[10] * mat1[10] + _data[14] * mat1[11]);
    mat[14] = float(_data[2] * mat1[12] + _data[6] * mat1[13] + 
                    _data[10] * mat1[14] + _data[14] * mat1[15]);

    // 4 row
    mat[3] = float(_data[3] * mat1[0] + _data[7] * mat1[1] + 
                   _data[11] * mat1[2] + _data[15] * mat1[3]);
    mat[7] = float(_data[3] * mat1[4] + _data[7] * mat1[5] + 
                   _data[11] * mat1[6] + _data[15] * mat1[7]);
    mat[11] = float(_data[3] * mat1[8] + _data[7] * mat1[9] + 
                    _data[11] * mat1[10] + _data[15] * mat1[11]);
    mat[15] = float(_data[3] * mat1[12] + _data[7] * mat1[13] + 
                    _data[11] * mat1[14] + _data[15] * mat1[15]);

    // set matrix
    set(mat);
}

void vrMatrix4x4f::multiply(const vrMatrix4x4f& m1, const vrMatrix4x4f& m2)
{
    float mat[16];
    float* mat1 = (float*) m1._data;
    float* mat2 = (float*) m2._data;

    // 1 row
    mat[0] = float(mat1[0] * mat2[0] + mat1[4] * mat2[1] + 
                   mat1[8] * mat2[2] + mat1[12] * mat2[3]);
    mat[4] = float(mat1[0] * mat2[4] + mat1[4] * mat2[5] + 
                   mat1[8] * mat2[6] + mat1[12] * mat2[7]);
    mat[8] = float(mat1[0] * mat2[8] + mat1[4] * mat2[9] + 
                   mat1[8] * mat2[10] + mat1[12] * mat2[11]);
    mat[12] = float(mat1[0] * mat2[12] + mat1[4] * mat2[13] + 
                    mat1[8] * mat2[14] + mat1[12] * mat2[15]);

    // 2 row
    mat[1] = float(mat1[1] * mat2[0] + mat1[5] * mat2[1] + 
                   mat1[9] * mat2[2] + mat1[13] * mat2[3]);
    mat[5] = float(mat1[1] * mat2[4] + mat1[5] * mat2[5] + 
                   mat1[9] * mat2[6] + mat1[13] * mat2[7]);
    mat[9] = float(mat1[1] * mat2[8] + mat1[5] * mat2[9] + 
                   mat1[9] * mat2[10] + mat1[13] * mat2[11]);
    mat[13] = float(mat1[1] * mat2[12] + mat1[5] * mat2[13] + 
                    mat1[9] * mat2[14] + mat1[13] * mat2[15]);

    // 3 row
    mat[2] = float(mat1[2] * mat2[0] + mat1[6] * mat2[1] + 
                   mat1[10] * mat2[2] + mat1[14] * mat2[3]);
    mat[6] = float(mat1[2] * mat2[4] + mat1[6] * mat2[5] + 
                   mat1[10] * mat2[6] + mat1[14] * mat2[7]);
    mat[10] = float(mat1[2] * mat2[8] + mat1[6] * mat2[9] + 
                    mat1[10] * mat2[10] + mat1[14] * mat2[11]);
    mat[14] = float(mat1[2] * mat2[12] + mat1[6] * mat2[13] + 
                    mat1[10] * mat2[14] + mat1[14] * mat2[15]);

    // 4 row
    mat[3] = float(mat1[3] * mat2[0] + mat1[7] * mat2[1] + 
                   mat1[11] * mat2[2] + mat1[15] * mat2[3]);
    mat[7] = float(mat1[3] * mat2[4] + mat1[7] * mat2[5] + 
                   mat1[11] * mat2[6] + mat1[15] * mat2[7]);
    mat[11] = float(mat1[3] * mat2[8] + mat1[7] * mat2[9] + 
                    mat1[11] * mat2[10] + mat1[15] * mat2[11]);
    mat[15] = float(mat1[3] * mat2[12] + mat1[7] * mat2[13] + 
                    mat1[11] * mat2[14] + mat1[15] * mat2[15]);

    // set matrix
    set(mat);
}

void vrMatrix4x4f::multiplyFast(const vrMatrix4x4f& m1, const vrMatrix4x4f& m2)
{
    float* mat1 = (float*) m1._data;
    float* mat2 = (float*) m2._data;

    // 1 row
    _data[0] = float(mat1[0] * mat2[0] + mat1[4] * mat2[1] + 
                     mat1[8] * mat2[2] + mat1[12] * mat2[3]);
    _data[4] = float(mat1[0] * mat2[4] + mat1[4] * mat2[5] + 
                     mat1[8] * mat2[6] + mat1[12] * mat2[7]);
    _data[8] = float(mat1[0] * mat2[8] + mat1[4] * mat2[9] + 
                     mat1[8] * mat2[10] + mat1[12] * mat2[11]);
    _data[12] = float(mat1[0] * mat2[12] + mat1[4] * mat2[13] + 
                      mat1[8] * mat2[14] + mat1[12] * mat2[15]);

    // 2 row
    _data[1] = float(mat1[1] * mat2[0] + mat1[5] * mat2[1] + 
                     mat1[9] * mat2[2] + mat1[13] * mat2[3]);
    _data[5] = float(mat1[1] * mat2[4] + mat1[5] * mat2[5] + 
                     mat1[9] * mat2[6] + mat1[13] * mat2[7]);
    _data[9] = float(mat1[1] * mat2[8] + mat1[5] * mat2[9] + 
                     mat1[9] * mat2[10] + mat1[13] * mat2[11]);
    _data[13] = float(mat1[1] * mat2[12] + mat1[5] * mat2[13] + 
                      mat1[9] * mat2[14] + mat1[13] * mat2[15]);

    // 3 row
    _data[2] = float(mat1[2] * mat2[0] + mat1[6] * mat2[1] + 
                     mat1[10] * mat2[2] + mat1[14] * mat2[3]);
    _data[6] = float(mat1[2] * mat2[4] + mat1[6] * mat2[5] + 
                     mat1[10] * mat2[6] + mat1[14] * mat2[7]);
    _data[10] = float(mat1[2] * mat2[8] + mat1[6] * mat2[9] + 
                      mat1[10] * mat2[10] + mat1[14] * mat2[11]);
    _data[14] = float(mat1[2] * mat2[12] + mat1[6] * mat2[13] + 
                      mat1[10] * mat2[14] + mat1[14] * mat2[15]);

    // 4 row
    _data[3] = float(mat1[3] * mat2[0] + mat1[7] * mat2[1] + 
                     mat1[11] * mat2[2] + mat1[15] * mat2[3]);
    _data[7] = float(mat1[3] * mat2[4] + mat1[7] * mat2[5] + 
                     mat1[11] * mat2[6] + mat1[15] * mat2[7]);
    _data[11] = float(mat1[3] * mat2[8] + mat1[7] * mat2[9] + 
                      mat1[11] * mat2[10] + mat1[15] * mat2[11]);
    _data[15] = float(mat1[3] * mat2[12] + mat1[7] * mat2[13] + 
                      mat1[11] * mat2[14] + mat1[15] * mat2[15]);
}

void vrMatrix4x4f::getRotation(vrRotation& rotation) 
{
    float c = (_data[0] + _data[5] + _data[10] - 1.0f) * 0.5f;

    rotation.setAxis(_data[6] - _data[9], _data[8] - _data[2], _data[1] - _data[4]);

    float len = sqrt(rotation.getX() * rotation.getX() + 
                     rotation.getY() * rotation.getY() + rotation.getZ() * rotation.getZ());

    float s = 0.5f * len;

    rotation.setAngle((float)atan2(s, c));

    if ( rotation.getX() == 0.0f && rotation.getY() == 0.0f && rotation.getZ() == 0.0f ) {
        rotation.set(0.0f, 1.0f, 0.0f, 0.0f);
    } else {
        len = 1.0f / len;
        rotation.setAxis(rotation.getX() * len, rotation.getY() * len, rotation.getZ() * len);
    }
}

void vrMatrix4x4f::invert()

{
    float det =
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

    if ( det == 0.0f ) return;
    det = 1 / det;

    float mat[16];

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



void vrMatrix4x4f::invert(const vrMatrix4x4f& mat)
{
    float* data = (float*) mat._data;

    float det =
        data[12] * data[9] * data[6] * data[3]-
        data[8] * data[13] * data[6] * data[3]-
        data[12] * data[5] * data[10] * data[3]+
        data[4] * data[13] * data[10] * data[3]+
        data[8] * data[5] * data[14] * data[3]-
        data[4] * data[9] * data[14] * data[3]-
        data[12] * data[9] * data[2] * data[7]+
        data[8] * data[13] * data[2] * data[7]+
        data[12] * data[1] * data[10] * data[7]-
        data[0] * data[13] * data[10] * data[7]-
        data[8] * data[1] * data[14] * data[7]+
        data[0] * data[9] * data[14] * data[7]+
        data[12] * data[5] * data[2] * data[11]-
        data[4] * data[13] * data[2] * data[11]-
        data[12] * data[1] * data[6] * data[11]+
        data[0] * data[13] * data[6] * data[11]+
        data[4] * data[1] * data[14] * data[11]-
        data[0] * data[5] * data[14] * data[11]-
        data[8] * data[5] * data[2] * data[15]+
        data[4] * data[9] * data[2] * data[15]+
        data[8] * data[1] * data[6] * data[15]-
        data[0] * data[9] * data[6] * data[15]-
        data[4] * data[1] * data[10] * data[15]+
        data[0] * data[5] * data[10] * data[15];

    if ( det == 0.0f ) return;
    det = 1 / det;

    float dstData[16];

    dstData[0] = (data[9]*data[14]*data[7] -
                  data[13]*data[10]*data[7] +
                  data[13]*data[6]*data[11] -
                  data[5]*data[14]*data[11] -
                  data[9]*data[6]*data[15] +
                  data[5]*data[10]*data[15]) * det;

    dstData[4] = (data[12]*data[10]*data[7] -
                  data[8]*data[14]*data[7] -
                  data[12]*data[6]*data[11] +
                  data[4]*data[14]*data[11] +
                  data[8]*data[6]*data[15] -
                  data[4]*data[10]*data[15]) * det;

    dstData[8] = (data[8]*data[13]*data[7] -
                  data[12]*data[9]*data[7] +
                  data[12]*data[5]*data[11] -
                  data[4]*data[13]*data[11] -
                  data[8]*data[5]*data[15] +
                  data[4]*data[9]*data[15]) * det;

    dstData[12] = (data[12]*data[9]*data[6] -
                   data[8]*data[13]*data[6] -
                   data[12]*data[5]*data[10] +
                   data[4]*data[13]*data[10] +
                   data[8]*data[5]*data[14] -
                   data[4]*data[9]*data[14]) * det;

    dstData[1] = (data[13]*data[10]*data[3] -
                  data[9]*data[14]*data[3] -
                  data[13]*data[2]*data[11] +
                  data[1]*data[14]*data[11] +
                  data[9]*data[2]*data[15] -
                  data[1]*data[10]*data[15]) * det;

    dstData[5] = (data[8]*data[14]*data[3] -
                  data[12]*data[10]*data[3] +
                  data[12]*data[2]*data[11] -
                  data[0]*data[14]*data[11] -
                  data[8]*data[2]*data[15] +
                  data[0]*data[10]*data[15]) * det;

    dstData[9] = (data[12]*data[9]*data[3] -
                  data[8]*data[13]*data[3] -
                  data[12]*data[1]*data[11] +
                  data[0]*data[13]*data[11] +
                  data[8]*data[1]*data[15] -
                  data[0]*data[9]*data[15]) * det;

    dstData[13] = (data[8]*data[13]*data[2] -
                   data[12]*data[9]*data[2] +
                   data[12]*data[1]*data[10] -
                   data[0]*data[13]*data[10] -
                   data[8]*data[1]*data[14] +
                   data[0]*data[9]*data[14]) * det;

    dstData[2] = (data[5]*data[14]*data[3] -
                  data[13]*data[6]*data[3] +
                  data[13]*data[2]*data[7] -
                  data[1]*data[14]*data[7] -
                  data[5]*data[2]*data[15] +
                  data[1]*data[6]*data[15]) * det;

    dstData[6] = (data[12]*data[6]*data[3] -
                  data[4]*data[14]*data[3] -
                  data[12]*data[2]*data[7] +
                  data[0]*data[14]*data[7] +
                  data[4]*data[2]*data[15] -
                  data[0]*data[6]*data[15]) * det;

    dstData[10] = (data[4]*data[13]*data[3] -
                   data[12]*data[5]*data[3] +
                   data[12]*data[1]*data[7] -
                   data[0]*data[13]*data[7] -
                   data[4]*data[1]*data[15] +
                   data[0]*data[5]*data[15]) * det;

    dstData[14] = (data[12]*data[5]*data[2] -
                   data[4]*data[13]*data[2] -
                   data[12]*data[1]*data[6] +
                   data[0]*data[13]*data[6] +
                   data[4]*data[1]*data[14] -
                   data[0]*data[5]*data[14]) * det;

    dstData[3] = (data[9]*data[6]*data[3] -
                  data[5]*data[10]*data[3] -
                  data[9]*data[2]*data[7] +
                  data[1]*data[10]*data[7] +
                  data[5]*data[2]*data[11] -
                  data[1]*data[6]*data[11]) * det;

    dstData[7] = (data[4]*data[10]*data[3] -
                  data[8]*data[6]*data[3] +
                  data[8]*data[2]*data[7] -
                  data[0]*data[10]*data[7] -
                  data[4]*data[2]*data[11] +
                  data[0]*data[6]*data[11]) * det;

    dstData[11] = (data[8]*data[5]*data[3] -
                   data[4]*data[9]*data[3] -
                   data[8]*data[1]*data[7] +
                   data[0]*data[9]*data[7] +
                   data[4]*data[1]*data[11] -
                   data[0]*data[5]*data[11]) * det;

    dstData[15] = (data[4]*data[9]*data[2] -
                   data[8]*data[5]*data[2] +
                   data[8]*data[1]*data[6] -
                   data[0]*data[9]*data[6] -
                   data[4]*data[1]*data[10] +
                   data[0]*data[5]*data[10]) * det;

    set(dstData);
}

void vrMatrix4x4f::invertFast(const vrMatrix4x4f& mat)
{
    float* srcData = (float*) mat._data;

    float det =
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

    if ( det == 0.0f ) return;
    det = 1 / det;

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

void vrMatrix4x4f::transpose()
{
    float m[16];

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

void vrMatrix4x4f::transposeFast(const vrMatrix4x4f& mat)
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

void vrMatrix4x4f::transpose(const vrMatrix4x4f& mat)
{
    float m[16];

    m[0] = mat._data[0];
    m[1] = mat._data[4];
    m[2] = mat._data[8];
    m[3] = mat._data[12];

    m[4] = mat._data[1];
    m[5] = mat._data[5];
    m[6] = mat._data[9];
    m[7] = mat._data[13];

    m[8] = mat._data[2];
    m[9] = mat._data[6];
    m[10] = mat._data[10];
    m[11] = mat._data[14];

    m[12] = mat._data[3];
    m[13] = mat._data[7];
    m[14] = mat._data[11];
    m[15] = mat._data[15];

    set(m);
}

void vrMatrix4x4f::set(double* m)
{
    for (int i = 0; i < 16; ++i) {
        _data[i] = (float) m[i];
    }
}

void vrMatrix4x4f::multiply(const vrMatrix4x4f& mat1, const vrVector3f& position)
{
    vrMatrix4x4f mat;
    mat.makeTranslation(position);
    multiply(mat1, mat);
}

void vrMatrix4x4f::multiply(const vrMatrix4x4f& mat1, const vrRotation& rotation)
{
    vrMatrix4x4f mat;
    mat.makeRotation(rotation);
    multiply(mat1, mat);
}

void vrMatrix4x4f::multiply(const vrVector3f& position, const vrMatrix4x4f& mat1)
{
    vrMatrix4x4f mat;
    mat.makeTranslation(position);
    multiply(mat, mat1);
}

void vrMatrix4x4f::multiply(const vrRotation& rotation, const vrMatrix4x4f& mat1)
{
    vrMatrix4x4f mat;
    mat.makeRotation(rotation);
    multiply(mat, mat1);
}

void vrMatrix4x4f::makeVecRotVec(const vrVector3f& vec1, const vrVector3f& vec2)
{
    vrVector3f axis;
    axis.cross(vec1, vec2);

    float angle = atan2(axis.length(), vec1.dot(vec2));
    if (axis.normalize() == 0.0f) {
        makeIdentity();
    } else {
        makeRotation(axis.x, axis.y, axis.z, angle);
    }
}

void vrMatrix4x4f::multiplyScale(const vrMatrix4x4f& mat1, const vrVector3f& scale)
{
    vrMatrix4x4f mat;
    mat.makeScale(scale);
    multiply(mat1, mat);
}

void vrMatrix4x4f::multiplyScale(const vrVector3f& scale, const vrMatrix4x4f& mat1)
{
    vrMatrix4x4f mat;
    mat.makeScale(scale);
    multiply(mat, mat1);
}
