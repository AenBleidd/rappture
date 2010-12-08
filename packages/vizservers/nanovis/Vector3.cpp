/*
 * ----------------------------------------------------------------------
 * Vector3.cpp : Vector class with 3 components
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
#include "Vector3.h"
#include "Mat4x4.h"
#include <math.h>
#include <stdio.h>

Vector3::Vector3(float _x, float _y, float _z){
	x = _x;
	y = _y;
	z = _z;
}


Vector3::Vector3(){}


Vector3 Vector3::operator +(Vector3 &op2){
	Vector3 ret;
	ret.x = x + op2.x;
	ret.y = y + op2.y;
	ret.z = z + op2.z;
	return ret;
}

Vector3 Vector3::operator -(Vector3 &op2){
	Vector3 ret;
	ret.x = x - op2.x;
	ret.y = y - op2.y;
	ret.z = z - op2.z;
	return ret;
}

float Vector3::operator *(Vector3 &op2){
	return x*op2.x +
		   y*op2.y +
		   z*op2.z;
}

float Vector3::dot(const Vector3& vec) const
{
	return x*vec.x +
		   y*vec.y +
		   z*vec.z;
}

bool Vector3::equal(Vector3 &op2){
	return (x==op2.x)&&(y==op2.y)&&(z==op2.z);
}

/*
float Vector3::operator *(Vector3 op2){
	return double(x)*double(op2.x) +
		   double(y)*double(op2.y) +
		   double(z)*double(op2.z);
}
*/

Vector3 Vector3::cross(Vector3 op2){
	return Vector3(double(y)*double(op2.z)-double(z)*double(op2.y), double(z)*double(op2.x)-double(x)*double(op2.z), double(x)*double(op2.y)-double(y)*double(op2.x));
}


Vector3 Vector3::operator *(float op2){
	Vector3 ret;
	ret.x = x*op2;
	ret.y = y*op2;
	ret.z = z*op2;
	return ret;
}


Vector3 Vector3::operator /(float op2){
	Vector3 ret;
	ret.x = x/double(op2);
	ret.y = y/double(op2);
	ret.z = z/double(op2);
	return ret;
}

//assign
void Vector3::operator<(Vector3 &op2){
	x = op2.x;
	y = op2.y;
	z = op2.z;
}

Vector3 Vector3::operator ^(Vector3 &op2){
	Vector3 ret;
	ret.x = y*op2.z-z*op2.y;
	ret.y = z*op2.x-x*op2.z;
	ret.z = x*op2.y-y*op2.x;
	return ret;
}

Vector3 Vector3::normalize(){
	Vector3 ret;
	float length = sqrt(x*x+y*y+z*z);
	ret.x = x/length;
	ret.y = y/length;
	ret.z = z/length;
	return ret;
}

Vector3 Vector3::rot_x(float degree){
	Vector3 ret;
	float rad = 3.1415926*degree/180.;
	ret.x = x;
	ret.y = y*cos(rad) - z*sin(rad);
	ret.z = y*sin(rad) + z*cos(rad);
	return ret;
}

Vector3 Vector3::rot_y(float degree){
	Vector3 ret;
	float rad = 3.1415926*degree/180.;
	ret.x = x*cos(rad) + z*sin(rad);
	ret.y = y;
	ret.z = -x*sin(rad) + z*cos(rad);
	return ret;
}

Vector3 Vector3::rot_z(float degree){
	Vector3 ret;
	float rad = 3.1415926*degree/180.;
	ret.x = x*cos(rad) - y*sin(rad);
	ret.y = x*sin(rad) + y*cos(rad);
	ret.z = z;
	return ret;
}

void Vector3::set(float newx, float newy, float newz){
	x = newx;
	y = newy;
	z = newz;
}

void Vector3::print(){
    TRACE("x:%f, y:%f, z:%f\n", x, y, z);
}

float Vector3::distance(Vector3 &another) const
{
	return sqrtf( (x - another.x) * (x - another.x)
				+ (y - another.y) * (y - another.y)
				+ (z - another.z) * (z - another.z) );
}

float Vector3::distanceSquare(Vector3 &another) const
{
	return ( (x - another.x) * (x - another.x)
				+ (y - another.y) * (y - another.y)
				+ (z - another.z) * (z - another.z) );
}

float Vector3::distanceSquare(float vx, float vy, float vz) const
{
	return ( (x - vx) * (x - vx)
				+ (y - vy) * (y - vy)
				+ (z - vz) * (z - vz) );
}

void Vector3::transform(const Vector3& v, const Mat4x4& mat)
{
    const float* m = mat.m;
    x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12];
    y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13];
    z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
}


float Vector3::length() const
{

    return sqrt(x * x + y * y + z * z);
}
