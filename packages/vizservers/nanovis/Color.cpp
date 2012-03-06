/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Color.cpp: Color class
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
#include <assert.h>

#include "Color.h"
#include "define.h"

Color::Color() {
	R=G=B=0.0;
	next=0;
}

Color::Color(double r, double g, double b) {
	R=r;
	G=g;
	B=b;
	next=0;
}

Color::Color(const Color& c)
 : R(c.R), G(c.G), B(c.B), next(c.next)
{
}

Color&
Color::operator=(const Color& c)
{
    R = c.R;
    G = c.G;
    B = c.B;
    next = c.next;
    return *this;
}

void Color::LimitColors(){ //Limits the color to be in range of 0.0 and 1.0
	if (R>1.0) R=1.0;
	if (G>1.0) G=1.0;
	if (B>1.0) B=1.0;

	if (R<0.0) R=0.0;
	if (G<0.0) G=0.0;
	if (B<0.0) B=0.0;
}

Color Color::operator*(double k){
	return Color(R*k, G*k, B*k);
}


//This is NOT member operator. It's used so we can write (k*V), not only (V*k) (V-vector k-scalar)
Color operator*(double k, Color &other){
	return Color(other.R*k, other.G*k, other.B*k);
}

Color::~Color(){}

Color Color::operator +(Color &other){
	return Color(this->R+other.R,this->G+other.G,this->B+other.B);
}

Color Color::operator *(Color &other){
	return Color(this->R*other.R,this->G*other.G,this->B*other.B);
}

void Color::GetRGBA(double opacity, unsigned char *result){
	LimitColors();

	assert(opacity>=0 && opacity <=1);

	result[0] = (unsigned char) (R*255.0);
	result[1] = (unsigned char) (G*255.0);
	result[2] = (unsigned char) (B*255.0);
	result[3] = (unsigned char) (opacity*255.0);
}

void Color::SetRGBA(unsigned char *color){
	R = color[0];
	G = color[1];
	B = color[2];
}

void Color::GetRGB(unsigned char *result){
	result[0] = (unsigned char) (R*255.0);
	result[1] = (unsigned char) (G*255.0);
	result[2] = (unsigned char) (B*255.0);
}

void Color::GetRGB(float *result){
	result[0] = (float) (R);
	result[1] = (float) (G);
	result[2] = (float) (B);
}

