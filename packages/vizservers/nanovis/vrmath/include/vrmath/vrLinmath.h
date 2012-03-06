/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#ifdef _WIN32
//#include <windows.h>

#ifdef LINMATHDLLEXPORT
#define LmExport __declspec(dllexport)
#define EXPIMP_TEMPLATE
#else
#define LmExport __declspec(dllimport)
#   define EXPIMP_TEMPLATE extern
#endif 

#else
#define LmExport   
#endif 


class vrVector3f;

extern vrVector3f LmExport vrCalcuNormal(const vrVector3f& v1, const vrVector3f& v2, const vrVector3f& v3);

#ifdef __cplusplus
extern "C" {
#endif

int LmExport vrUnproject(float winx, float winy, float winz,
                const float modelMatrix[16],
                const float projMatrix[16],
                const int viewport[4],
                float *objx, float *objy, float *objz);

void LmExport vrPerspective(float fovy, float aspect, float zNear, float zFar, float* matrix);

#ifdef __cplusplus
};
#endif


