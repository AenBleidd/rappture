/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Color.h: RGBA color class
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
#ifndef COLOR_H 
#define COLOR_H

class Color  
{
public:
    Color();

    Color(double r, double g, double b);

    ~Color();

    Color(const Color& c);

    Color& operator=(const Color& c);

    void getRGB(unsigned char *result);

    void getRGB(float *result);

    /// Limits the color to be in range of [0,1]
    void clamp();

    Color operator*(const Color& other) const;

    Color operator*(double k) const;

    Color operator+(const Color& other) const;

    friend Color operator*(double k, const Color& other);

private:
    double _r;	///< Red component
    double _g;	///< Green component
    double _b;	///< Blue component
};

// This is NOT member operator. It's used so we can write (k*V), 
// not only (V*k) (V-vector k-scalar)
inline Color operator*(double k, const Color& c)
{
    return Color(c._r * k, c._g * k, c._b * k);
}

#endif
