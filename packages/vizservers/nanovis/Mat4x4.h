/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Mat4x4.h: Mat4x4 class
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
#ifndef _MAT4X4_H_
#define _MAT4X4_H_

#include "Vector4.h"

class Mat4x4  
{
        
public:
    float m[16];        //row by row
    Mat4x4() {
	/*empty*/
    };
    Mat4x4(float *vals);

    void print(void);
    Mat4x4 inverse(void);
    Mat4x4 transpose(void);
    Vector4 multiply_row_vector(Vector4 v);
    Vector4 transform(Vector4 v);
    Mat4x4 operator*(Mat4x4 op);
};

#endif
