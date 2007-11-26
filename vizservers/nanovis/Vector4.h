/*
 * ----------------------------------------------------------------------
 * Vector4.h: Vector4 class
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
#ifndef _VECTOR4_H_ 
#define _VECTOR4_H_

class Vector4  
{
public:
	float x, y, z, w;

	Vector4();
	Vector4(float _x, float _y, float _z, float _w);
	~Vector4();

	void perspective_devide();
	void print();
	Vector4 operator +(Vector4 &op2); //plus 	
	Vector4 operator -(Vector4 &op2); //minus
	float operator *(Vector4 &op2);	  //dot product
	Vector4 operator*(float op2);	  //mul per component
	Vector4 operator/(float op2);	  //mul per component
	void operator <(Vector4 &op2);	  //assign	

    void set(float x1, float y1, float z1, float w1);
};

#endif
