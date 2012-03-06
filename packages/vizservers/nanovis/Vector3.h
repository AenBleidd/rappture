/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Vector3.h : Vector class with 3 components
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef _VECTOR3_H_
#define _VECTOR3_H_

/*
 * FIXME: The files that explicitly use the "rot*", "length", "distance", or
 * "normalize" methods, should include the following headers.  Don't do it
 * here.
 */
#include <math.h> 
#include "Mat4x4.h"

class Vector3{
private:
    float radians(float degree) const {
	return (M_PI * degree) / 180.0;
    }
public:
    float x, y, z;
    Vector3(void) {
	/*empty*/
    }
    Vector3(float x_val, float y_val, float z_val) {
	set(x_val, y_val, z_val);
    }
    Vector3 operator +(float scalar) {
	return Vector3(x + scalar, y + scalar, z + scalar);
    }
    Vector3 operator -(float scalar) {
	return Vector3(x - scalar, y - scalar, z - scalar);
    }
    Vector3 operator *(float scalar) {
	return Vector3(x * scalar, y * scalar, z * scalar);
    }
    Vector3 operator /(float scalar) {
	return Vector3(x / scalar, y / scalar, z / scalar);
    }
    Vector3 operator +(Vector3 &op2) {
	return Vector3(x + op2.x, y + op2.y, z + op2.z);
    }
    Vector3 operator -(Vector3 &op2) {
	return Vector3(x - op2.x, y - op2.y, z - op2.z);
    }
    float operator *(const Vector3 &op2){
	return x*op2.x + y*op2.y + z*op2.z;
    }
    float dot(const Vector3& vec) const {
	return x*vec.x + y*vec.y + z*vec.z;
    }
    bool equal(Vector3 &op2) {
	return (x==op2.x) && (y==op2.y) && (z==op2.z);
    }
    Vector3 cross(Vector3 op2) {
	return Vector3(y*op2.z - z*op2.y, z*op2.x - x*op2.z, x*op2.y - y*op2.x);
    }
    void operator<(Vector3 &op2) {
	set(op2.x, op2.y, op2.z);
    }
    Vector3 operator ^(Vector3 &op2) {
	return cross(op2);
    }
    Vector3 normalize(void) {
	float len = length();
	return Vector3(x / len, y / len, z / len);
    }
    Vector3 rot_x(float degree) {
	float rad = radians(degree);
	return Vector3(x, 
		       y*cos(rad) - z*sin(rad), 
		       y*sin(rad) + z*cos(rad));
    }
    Vector3 rot_y(float degree) {
	float rad = radians(degree);
	return Vector3(x*cos(rad) + z*sin(rad), 
		       y, 
		       -x*sin(rad) + z*cos(rad));
    }
    Vector3 rot_z(float degree) {
	float rad = radians(degree);
	return Vector3(x*cos(rad) - y*sin(rad), 
		       x*sin(rad) + y*cos(rad), 
		       z);
    }
    void set(float x_val, float y_val, float z_val) {
	x = x_val, y = y_val, z = z_val;
    }
    void print(void){
	TRACE("(x:%f, y:%f, z:%f)\n", x, y, z);
    }
    float distance(Vector3 &v) const {
	return sqrtf(distanceSquare(v));
    }
    float distanceSquare(Vector3 &v) const {
	return (x-v.x)*(x-v.x) + (y-v.y)*(y-v.y) + (z-v.z)*(z-v.z);
    }
    float distanceSquare(float vx, float vy, float vz) const {
	return (x-vx)*(x-vx) + (y-vy)*(y-vy) + (z-vz)*(z-vz);
    }
    void transform(const Vector3& v, const Mat4x4& mat) {
	const float* m = mat.m;
	x = m[0] * v.x + m[4] * v.y + m[8]  * v.z + m[12];
	y = m[1] * v.x + m[5] * v.y + m[9]  * v.z + m[13];
	z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
    }
    float length(void) const {
	return sqrt(x*x + y*y + z*z);
    }

    Vector3 scale(const Vector3& scale)
    {
        Vector3 v;
        v.x = x * scale.x;
        v.y = y * scale.y;
        v.z = z * scale.z;
        return v;
    }

    friend Vector3 operator+(const Vector3& value1, const Vector3& value2);


};

inline Vector3 operator+(const Vector3& value1, const Vector3& value2)
{
	return Vector3(value1.x + value2.x, value1.y + value2.y, value1.z + value2.z);
}

#endif
