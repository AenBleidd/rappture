/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VR3D_H
#define VR3D_H

#include <GL/glew.h>

#ifdef __cplusplus
extern "C" {
#endif

void vrInitializeTimeStamp();

float vrGetTimeStamp();

void vrUpdateTimeStamp();

void vrInit();

void vrExit();

#ifdef __cplusplus
}
#endif

#endif
