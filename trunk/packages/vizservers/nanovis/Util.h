/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __UTIL_H__
#define __UTIL_H__

#include <Vector3.h>

extern void evaluate(float fraction, float* key, int count, Vector3* keyValue, Vector3* ret);
extern void evaluate(float fraction, float* key, int count, float* keyValue, float* ret);

#endif
