/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef DX_READER_COMMON_H
#define DX_READER_COMMON_H

extern float *
merge(float *scalar, float *gradient, int size);

extern void
normalizeScalar(float *fdata, int count, float min, float max);

extern float *
computeGradient(float *fdata,
                int width, int height, int depth,
                float dx, float dy, float dz,
                float min, float max);

extern void
computeSimpleGradient(float *data, int nx, int ny, int nz,
                      float dx = 1.0f, float dy = 1.0f, float dz = 1.0f);

#endif
