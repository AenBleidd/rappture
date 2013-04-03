/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_GRADIENT_FILTER_H
#define NV_GRADIENT_FILTER_H

namespace nv {

typedef enum {
    DATRAW_UCHAR,
    DATRAW_FLOAT,
    DATRAW_USHORT
} DataType;

extern void computeGradients(float *gradients, void *volData,
                             int *sizes, float *spacing, DataType dataType);

extern void filterGradients(float *gradients, int *sizes);

extern void quantizeGradients(float *gradientsIn, void *gradientsOut,
                              int *sizes, DataType dataType);

}

#endif 
