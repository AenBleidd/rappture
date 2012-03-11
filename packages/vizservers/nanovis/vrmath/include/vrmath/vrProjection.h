/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRPROJECTION_H
#define VRPROJECTION_H

#ifdef __cplusplus
extern "C" {
#endif

LmExport int
unproject(float winx, float winy, float winz,
          const float modelMatrix[16],
          const float projMatrix[16],
          const int viewport[4],
          float *objx, float *objy, float *objz);

LmExport void
perspective(float fovy, float aspect, float zNear, float zFar, float *matrix);

#ifdef __cplusplus
};
#endif

#endif

