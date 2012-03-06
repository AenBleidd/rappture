/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * Quaternion code by BLACKAXE / kolor aka Laurent Schmalen
 *
 * I have changed names according to my naming rules
 */

#include <cstdlib>
#include <cmath>
#include <vrmath/vrQuaternion.h>
#include <vrmath/vrRotation.h>
#include <vrmath/vrLinmath.h>
#include <vrmath/vrVector3f.h>
#include <float.h>

#ifndef PI
#define PI 3.14159264
#endif

vrQuaternion::vrQuaternion()
: x(1.0f), y(0.0f), z(0.0f), w(0.0f)
{
}

vrQuaternion::vrQuaternion(const vrRotation& rot)
{
	set(rot);
}

vrQuaternion::vrQuaternion(float x1, float y1, float z1, float w1)
: x(x1), y(y1), z(z1), w(w1)
{
}

const vrQuaternion& vrQuaternion::normalize()
{
	float norme = sqrt(w*w + x*x + y*y + z*z);
	if (norme == 0.0)
	{
		w = 1.0; 
		x = y = z = 0.0f;
	}
  	else
    	{
	  	float recip = 1.0f/norme;
	  	w *= recip;
	  	x *= recip;
	  	y *= recip;
	  	z *= recip;
	}

  	return *this;
}

const vrQuaternion& vrQuaternion::set(const vrRotation& rot)
{
	/*
	float omega, s, c;

  	s = sqrt(rot.x*rot.x + rot.y*rot.y + rot.z*rot.z);
  
	if (fabs(s) > FLT_EPSILON)
    	{
	  c = 1.0/s;
	  float rot_x = rot.x * c;
	  float rot_y = rot.y * c;
	  float rot_z = rot.z * c;

	  omega = -0.5f * rot.angle;
	  s = (float)sin(omega);
	  x = s*rot_x;
	  y = s*rot_y;
	  z = s*rot_z;
	  w = (float)cos(omega);
	}
  	else
    	{
	  x = y = 0.0f;
	  z = 0.0f;
	  w = 1.0f;
	}

	normalize();
  	
	return *this;
	*/
	vrVector3f q(rot.x, rot.y, rot.z);
	q.normalize();
	float s = (float)sin(rot.angle * 0.5);
	x = s * q.x;
	y = s * q.y;
	z = s * q.z;
	w = (float) cos (rot.angle * 0.5f);

	return *this;
}

void vrQuaternion::slerp(const vrQuaternion &a,const vrQuaternion &b, const float t)
{
	/*
  	float omega, cosom, sinom, sclp, sclq;
  	cosom = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;

	if ((1.0f+cosom) > FLT_EPSILON)
	{
	  	if ((1.0f-cosom) > FLT_EPSILON)
		{
		  omega = acos(cosom);
		  sinom = sin(omega);
		  sclp = sin((1.0f-t)*omega) / sinom;
		  sclq = sin(t*omega) / sinom;
		}
	  	else
		{
		  sclp = 1.0f - t;
		  sclq = t;
		}

		x = sclp*a.x + sclq*b.x;
	  	y = sclp*a.y + sclq*b.y;
	  	z = sclp*a.z + sclq*b.z;
	  	w = sclp*a.w + sclq*b.w;
	}
  	else
	{
	  	x =-a.y;
	  	y = a.x;
	  	z =-a.w;
	  	w = a.z;

	  	sclp = sin((1.0f-t) * PI * 0.5);
	  	sclq = sin(t * PI * 0.5);

	  	x = sclp*a.x + sclq*b.x;
	  	y = sclp*a.y + sclq*b.y;
	  	z = sclp*a.z + sclq*b.z;
	}
	*/
	double alpha, beta;
	double cosom = a.x * b.x + a.y * b.y + 
					a.z * b.z + a.w * b.w;

	bool flip = (cosom < 0.0);
	if (flip) cosom = -cosom;
	if ((1.0 - cosom) > 0.00001f){
		double omega = acos(cosom);
		double sinom = sin(omega);
		alpha = sin((1.0 - t) * omega) / sinom;
		beta = sin(t * omega) / sinom;
	}
	else {
		alpha = 1.0f - t;
		beta = t;
	}

	if (flip) beta = -beta;
	x = (float) (alpha * a.x + beta * b.x);
	y = (float) (alpha * a.y + beta * b.y);
	z = (float) (alpha * a.z + beta * b.z);
	w = (float) (alpha * a.w + beta * b.w);
}

