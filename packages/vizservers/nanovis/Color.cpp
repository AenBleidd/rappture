/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Color.cpp: Color class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <stdio.h>
#include <assert.h>

#include "Color.h"

Color::Color() :
    _r(0),
    _g(0),
    _b(0)
{
}

Color::Color(double r, double g, double b) :
    _r(r),
    _g(g),
    _b(b)
{
}

Color::Color(const Color& c) :
    _r(c._r),
    _g(c._g),
    _b(c._b)
{
}

Color::~Color()
{
}

Color&
Color::operator=(const Color& c)
{
    _r = c._r;
    _g = c._g;
    _b = c._b;
    return *this;
}

// Limits the color to be in range of [0,1]
void Color::clamp()
{
    if (_r > 1.0) _r = 1.0;
    if (_g > 1.0) _g = 1.0;
    if (_b > 1.0) _b = 1.0;

    if (_r < 0.0) _r = 0.0;
    if (_g < 0.0) _g = 0.0;
    if (_b < 0.0) _b = 0.0;
}

Color Color::operator*(double k) const
{
    return Color(_r * k, _g * k, _b * k);
}

Color Color::operator*(const Color& other) const
{
    return Color(_r * other._r, _g * other._g, _b * other._b);
}

Color Color::operator+(const Color& other) const
{
    return Color(_r + other._r, _g + other._g, _b + other._b);
}

void Color::getRGB(unsigned char *result)
{
    result[0] = (unsigned char)(_r*255.0);
    result[1] = (unsigned char)(_g*255.0);
    result[2] = (unsigned char)(_b*255.0);
}

void Color::getRGB(float *result)
{
    result[0] = (float)_r;
    result[1] = (float)_g;
    result[2] = (float)_b;
}
