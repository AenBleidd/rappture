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

class Vector3{

public:
	float x, y, z;
	Vector3(float _x, float _y, float _z);
	Vector3();

	Vector3 operator +(Vector3 &op2); 	
	Vector3 operator -(Vector3 &op2);
	bool equal(Vector3 &op2);
	//float operator *(Vector3 &op2);	//dot product
	float operator *(Vector3 &op2);		//dot product
	Vector3 operator *(float op2);	  //mul per component
	Vector3 operator /(float op2);	  //mul per component
	void operator <(Vector3 &op2);	  //assign	
	Vector3 operator ^(Vector3 &op2); //cross product
	Vector3 normalize();
	Vector3 rot_x(float degree);	  //rotation about x
	Vector3 rot_y(float degree);	  //rotation about y
	Vector3 rot_z(float degree);	  //rotation about z
	void set(float newx, float newy, float newz);
	void print();
	float distance(Vector3 &another);	//compute distance

	//fast version
	Vector3 cross(Vector3 op2);
};

#endif
