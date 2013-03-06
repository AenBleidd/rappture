/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRLINMATH_H
#define VRLINMATH_H

class vrVector3f;

extern vrVector3f vrCalcuNormal(const vrVector3f& v1, const vrVector3f& v2, const vrVector3f& v3);

#ifdef __cplusplus
extern "C" {
#endif

int vrUnproject(float winx, float winy, float winz,
                const float modelMatrix[16],
                const float projMatrix[16],
                const int viewport[4],
                float *objx, float *objy, float *objz);

void vrPerspective(float fovy, float aspect, float zNear, float zFar, float* matrix);

#ifdef __cplusplus
};
#endif

#endif

