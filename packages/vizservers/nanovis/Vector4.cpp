/*
 * ----------------------------------------------------------------------
 * Vector4.cpp: Vector4 class
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
#include <stdio.h>
#include "Vector4.h"


Vector4::Vector4(){}

Vector4::Vector4(float _x, float _y, float _z, float _w){
	x = _x; 
	y = _y; 
	z = _z; 
	w = _w;
}

Vector4::~Vector4(){}

void Vector4::print(){
    TRACE("Vector4: (%.3f, %.3f, %.3f, %.3f)\n", x, y, z, w);
}

void Vector4::perspective_devide(){
	x /= w;
	y /= w;
	z /= w;
	w = 1.;
}

Vector4 Vector4::operator +(Vector4 &op2){
	Vector4 ret;
	ret.x = x + op2.x;
	ret.y = y + op2.y;
	ret.z = z + op2.z;
	ret.w = w + op2.w;
	return ret;
}

Vector4 Vector4::operator -(Vector4 &op2){
	Vector4 ret;
	ret.x = x - op2.x;
	ret.y = y - op2.y;
	ret.z = z - op2.z;
	ret.w = w - op2.w;
	return ret;
}

float Vector4::operator *(Vector4 &op2){
	return x*op2.x +
		   y*op2.y +
		   z*op2.z +
		   w*op2.w;
}

Vector4 Vector4::operator *(float op2){
	Vector4 ret;
	ret.x = x*op2;
	ret.y = y*op2;
	ret.z = z*op2;
	ret.w = w*op2;
	return ret;
}

Vector4 Vector4::operator /(float op2){
	Vector4 ret;
	ret.x = x/op2;
	ret.y = y/op2;
	ret.z = z/op2;
	ret.w = w/op2;
	return ret;
}

//assign
void Vector4::operator <(Vector4 &op2){
	x = op2.x;
	y = op2.y;
	z = op2.z;
	w = op2.w;
}

void Vector4::set(float x1, float y1, float z1, float w1)
{
	x = x1;
	y = y1;
	z = z1;
	w = w1;
}
