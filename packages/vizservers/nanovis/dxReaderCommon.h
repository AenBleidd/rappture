/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef DX_READER_COMMON_H
#define DX_READER_COMMON_H

extern float *
merge(float *scalar, float *gradient, int size);

extern void
normalizeScalar(float *fdata, int count, float min, float max);

extern float *
computeGradient(float *fdata, int width, int height, int depth,
                float min, float max);

extern void
computeSimpleGradient(float *data, int nx, int ny, int nz);

#endif
