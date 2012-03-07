/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Mat4x4.cpp: Mat4x4 class
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
#include <math.h>

#include "Mat4x4.h"

Mat4x4::Mat4x4(float *vals)
{
    for (int i = 0; i < 16; i++) {
        m[i] = vals[i];
    }
}

void
Mat4x4::print() const
{
    TRACE("%f %f %f %f\n", m[0], m[1], m[2], m[3]);
    TRACE("%f %f %f %f\n", m[4], m[5], m[6], m[7]);
    TRACE("%f %f %f %f\n", m[8], m[9], m[10], m[11]);
    TRACE("%f %f %f %f\n", m[12], m[13], m[14], m[15]);
}

Mat4x4
Mat4x4::inverse() const
{
    Mat4x4 p;

    float m00 = m[0], m01 = m[4], m02 = m[8], m03 = m[12];
    float m10 = m[1], m11 = m[5], m12 = m[9], m13 = m[13];
    float m20 = m[2], m21 = m[6], m22 = m[10], m23 = m[14];
    float m30 = m[3], m31 = m[7], m32 = m[11], m33 = m[15];

    float det0, inv0;

#define det33(a1,a2,a3,b1,b2,b3,c1,c2,c3) \
    (a1*(b2*c3-b3*c2) + b1*(c2*a3-a2*c3) + c1*(a2*b3-a3*b2))

    float cof00 =  det33(m11, m12, m13, m21, m22, m23, m31, m32, m33);
    float cof01 = -det33(m12, m13, m10, m22, m23, m20, m32, m33, m30);
    float cof02 =  det33(m13, m10, m11, m23, m20, m21, m33, m30, m31);
    float cof03 = -det33(m10, m11, m12, m20, m21, m22, m30, m31, m32);

    float cof10 = -det33(m21, m22, m23, m31, m32, m33, m01, m02, m03);
    float cof11 =  det33(m22, m23, m20, m32, m33, m30, m02, m03, m00);
    float cof12 = -det33(m23, m20, m21, m33, m30, m31, m03, m00, m01);
    float cof13 =  det33(m20, m21, m22, m30, m31, m32, m00, m01, m02);

    float cof20 =  det33(m31, m32, m33, m01, m02, m03, m11, m12, m13);
    float cof21 = -det33(m32, m33, m30, m02, m03, m00, m12, m13, m10);
    float cof22 =  det33(m33, m30, m31, m03, m00, m01, m13, m10, m11);
    float cof23 = -det33(m30, m31, m32, m00, m01, m02, m10, m11, m12);

    float cof30 = -det33(m01, m02, m03, m11, m12, m13, m21, m22, m23);
    float cof31 =  det33(m02, m03, m00, m12, m13, m10, m22, m23, m20);
    float cof32 = -det33(m03, m00, m01, m13, m10, m11, m23, m20, m21);
    float cof33 =  det33(m00, m01, m02, m10, m11, m12, m20, m21, m22);

#undef det33

#if 0
    TRACE("Invert:\n");
    TRACE("   %12.9f %12.9f %12.9f %12.9f\n", m00, m01, m02, m03);
    TRACE("   %12.9f %12.9f %12.9f %12.9f\n", m10, m11, m12, m13);
    TRACE("   %12.9f %12.9f %12.9f %12.9f\n", m20, m21, m22, m23);
    TRACE("   %12.9f %12.9f %12.9f %12.9f\n", m30, m31, m32, m33);
#endif

#if 0
    det0  = m00 * cof00 + m01 * cof01 + m02 * cof02 + m03 * cof03;
#else
    det0  = m30 * cof30 + m31 * cof31 + m32 * cof32 + m33 * cof33;
#endif
    inv0  = (det0 == 0.0f) ? 0.0f : 1.0f / det0;

    p.m[0] = cof00 * inv0;
    p.m[1] = cof01 * inv0;
    p.m[2] = cof02 * inv0;
    p.m[3] = cof03 * inv0;

    p.m[4] = cof10 * inv0;
    p.m[5] = cof11 * inv0;
    p.m[6] = cof12 * inv0;
    p.m[7] = cof13 * inv0;

    p.m[8] = cof20 * inv0;
    p.m[9] = cof21 * inv0;
    p.m[10] = cof22 * inv0;
    p.m[11] = cof23 * inv0;

    p.m[12] = cof30 * inv0;
    p.m[13] = cof31 * inv0;
    p.m[14] = cof32 * inv0;
    p.m[15]= cof33 * inv0;	

    return p;
}

Vector4
Mat4x4::multiply_row_vector(const Vector4& v) const
{
    Vector4 ret;
    ret.x = (m[0]  * v.x) + (m[1]  * v.y) + (m[2]  * v.z) + (m[3]  * v.w);
    ret.y = (m[4]  * v.x) + (m[5]  * v.y) + (m[6]  * v.z) + (m[7]  * v.w);
    ret.z = (m[8]  * v.x) + (m[9]  * v.y) + (m[10] * v.z) + (m[11] * v.w);
    ret.w = (m[12] * v.x) + (m[13] * v.y) + (m[14] * v.z) + (m[15] * v.w);
    return ret;
}

Vector4
Mat4x4::transform(const Vector4& v) const
{
    Vector4 ret;
    ret.x = v.x*m[0]+v.y*m[4]+v.z*m[8]+v.w*m[12];
    ret.y = v.x*m[1]+v.y*m[5]+v.z*m[9]+v.w*m[13];
    ret.z = v.x*m[2]+v.y*m[6]+v.z*m[10]+v.w*m[14];
    ret.w = v.x*m[3]+v.y*m[7]+v.z*m[11]+v.w*m[15];
    return ret;
}

Mat4x4 
Mat4x4::operator *(const Mat4x4& mat) const
{
    Mat4x4 ret;
    
    ret.m[0] = m[0]*mat.m[0]+m[1]*mat.m[4]+m[2]*mat.m[8]+m[3]*mat.m[12];
    ret.m[1] = m[0]*mat.m[1]+m[1]*mat.m[5]+m[2]*mat.m[9]+m[3]*mat.m[13];
    ret.m[2] = m[0]*mat.m[2]+m[1]*mat.m[6]+m[2]*mat.m[10]+m[3]*mat.m[14];
    ret.m[3] = m[0]*mat.m[3]+m[1]*mat.m[7]+m[2]*mat.m[11]+m[3]*mat.m[15];
    
    ret.m[4] = m[4]*mat.m[0]+m[5]*mat.m[4]+m[6]*mat.m[8]+m[7]*mat.m[12];
    ret.m[5] = m[4]*mat.m[1]+m[5]*mat.m[5]+m[6]*mat.m[9]+m[7]*mat.m[13];
    ret.m[6] = m[4]*mat.m[2]+m[5]*mat.m[6]+m[6]*mat.m[10]+m[7]*mat.m[14];
    ret.m[7] = m[4]*mat.m[3]+m[5]*mat.m[7]+m[6]*mat.m[11]+m[7]*mat.m[15];
    
    ret.m[8] = m[8]*mat.m[0]+m[9]*mat.m[4]+m[10]*mat.m[8]+m[11]*mat.m[12];
    ret.m[9] = m[8]*mat.m[1]+m[9]*mat.m[5]+m[10]*mat.m[9]+m[11]*mat.m[13];
    ret.m[10] = m[8]*mat.m[2]+m[9]*mat.m[6]+m[10]*mat.m[10]+m[11]*mat.m[14];
    ret.m[11] = m[8]*mat.m[3]+m[9]*mat.m[7]+m[10]*mat.m[11]+m[11]*mat.m[15];
    
    ret.m[12] = m[12]*mat.m[0]+m[13]*mat.m[4]+m[14]*mat.m[8]+m[15]*mat.m[12];
    ret.m[13] = m[12]*mat.m[1]+m[13]*mat.m[5]+m[14]*mat.m[9]+m[15]*mat.m[13];
    ret.m[14] = m[12]*mat.m[2]+m[13]*mat.m[6]+m[14]*mat.m[10]+m[15]*mat.m[14];
    ret.m[15] = m[12]*mat.m[3]+m[13]*mat.m[7]+m[14]*mat.m[11]+m[15]*mat.m[15];
    
    return ret;
}

Mat4x4
Mat4x4::transpose() const
{
    Mat4x4 ret;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            ret.m[j+4*i] = this->m[i+4*j];
        }
    }
    return ret;
}
