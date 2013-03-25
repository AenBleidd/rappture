/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include "ReaderCommon.h"
#include "GradientFilter.h"

#include <vrmath/Vector3f.h>

#include "stdlib.h"

using namespace vrmath;

float *
merge(float *scalar, float *gradient, int size)
{
    float *data = (float *)malloc(sizeof(float) * 4 * size);

    Vector3f *g = (Vector3f *)gradient;

    int ngen = 0, sindex = 0;
    for (sindex = 0; sindex < size; ++sindex) {
        data[ngen++] = scalar[sindex];
        data[ngen++] = 1.0 - g[sindex].x;
        data[ngen++] = 1.0 - g[sindex].y;
        data[ngen++] = 1.0 - g[sindex].z;
    }
    return data;
}

/**
 * \brief Normalize data to [0,1] based on vmin,vmax range
 * 
 * Data outside of given range is clamped, and NaNs are set to
 * -1 in the output
 *
 * \param data Float array of unnormalized data, will be normalized on return
 * \param count Number of elts in array
 * \param stride Stride between values in data array
 * \param vmin Minimum value in data array
 * \param vmax Maximum value in data array
 */
void
normalizeScalar(float *data, int count, int stride, double vmin, double vmax)
{
    double dv = vmax - vmin;
    dv = (dv == 0.0) ? 1.0 : dv;
    for (int pt = 0, i = 0; pt < count; ++pt, i += stride) {
        double v = data[i];
        if (isnan(v)) {
            data[i] = -1.0f;
        } else if (v < vmin) {
            data[i] = 0.0f;
        } else if (v > vmax) {
            data[i] = 1.0f;
        } else {
            data[i] = (float)((v - vmin)/ dv);
        }
    }
}

/**
 * \brief Compute Sobel filtered gradients for a 3D volume
 *
 * This technique is fairly expensive in terms of memory and
 * running time due to the filter extent.
 *
 * \param data Data array with X the fastest running, stride of
 * 1 float
 * \param nx number of values in X direction
 * \param ny number of values in Y direction
 * \param nz number of values in Z direction
 * \param dx Size of voxels in X direction
 * \param dy Size of voxels in Y direction
 * \param dz Size of voxels in Z direction
 * \param min Minimum value in data
 * \param max Maximum value in data
 * \return Returns a float array with stride of 4 floats 
 * containing the normalized scalar and the x,y,z components of 
 * the (normalized) gradient vector
 */
float *
computeGradient(float *data,
                int nx, int ny, int nz,
                float dx, float dy, float dz,
                float min, float max)
{
    int npts = nx * ny * nz;
    float *gradients = (float *)malloc(npts * 3 * sizeof(float));
    float *tempGradients = (float *)malloc(npts * 3 * sizeof(float));
    int sizes[3] = { nx, ny, nz };
    float spacing[3] = { dx, dy, dz };
    computeGradients(tempGradients, data, sizes, spacing, DATRAW_FLOAT);
    filterGradients(tempGradients, sizes);
    quantizeGradients(tempGradients, gradients, sizes, DATRAW_FLOAT);
    free(tempGradients);
    normalizeScalar(data, npts, 1, min, max);
    float *dataOut = merge(data, gradients, npts);
    free(gradients);
    return dataOut;
}

/**
 * \brief Compute gradients for a 3D volume with cubic cells
 * 
 * The gradients are estimated using the central difference
 * method.  This function assumes the data are normalized
 * to [0,1] with missing data/NaNs represented by a negative
 * value.
 *
 * \param data Data array with X the fastest running. There
 * should be 4 floats allocated for each node, with the 
 * first float containing the scalar value.  The subsequent
 * 3 floats will be filled with the x,y,z components of the
 * gradient vector
 * \param nx The number of nodes in the X direction
 * \param ny The number of nodes in the Y direction
 * \param nz The number of nodes in the Z direction
 * \param dx The spacing (cell length) in the X direction
 * \param dy The spacing (cell length) in the Y direction
 * \param dz The spacing (cell length) in the Z direction
 */
void
computeSimpleGradient(float *data,
                      int nx, int ny, int nz,
                      float dx, float dy, float dz)
{
    bool clampToEdge = true;
    double borderVal = 0.0;

#define BORDER ((clampToEdge ? data[ngen] : borderVal))

    // Compute the gradient of this data.  BE CAREFUL: center
    // calculation on each node to avoid skew in either direction.
    int ngen = 0;
    for (int iz = 0; iz < nz; iz++) {
        for (int iy = 0; iy < ny; iy++) {
            for (int ix = 0; ix < nx; ix++) {
                // gradient in x-direction
                double valm1 = (ix == 0) ? BORDER : data[ngen - 4];
                double valp1 = (ix == nx-1) ? BORDER : data[ngen + 4];
                if (valm1 < 0.0 || valp1 < 0.0) {
                    data[ngen+1] = 0.0;
                } else {
                    data[ngen+1] = -(valp1-valm1)/(2. * dx);
                 }

                // gradient in y-direction
                valm1 = (iy == 0) ? BORDER : data[ngen - 4*nx];
                valp1 = (iy == ny-1) ? BORDER : data[ngen + 4*nx];
                if (valm1 < 0.0 || valp1 < 0.0) {
                    data[ngen+2] = 0.0;
                } else {
                    data[ngen+2] = -(valp1-valm1)/(2. * dy);
                }

                // gradient in z-direction
                valm1 = (iz == 0) ? BORDER : data[ngen - 4*nx*ny];
                valp1 = (iz == nz-1) ? BORDER : data[ngen + 4*nx*ny];
                if (valm1 < 0.0 || valp1 < 0.0) {
                    data[ngen+3] = 0.0;
                } else {
                    data[ngen+3] = -(valp1-valm1)/(2. * dz);
                }
                // Normalize and scale/bias to [0,1] range
                // The volume shader will expand to [-1,1]
                double len = sqrt(data[ngen+1]*data[ngen+1] +
                                  data[ngen+2]*data[ngen+2] +
                                  data[ngen+3]*data[ngen+3]);
                if (len < 1.0e-6) {
                    data[ngen+1] = 0.0;
                    data[ngen+2] = 0.0;
                    data[ngen+3] = 0.0;
                } else {
                    data[ngen+1] = (data[ngen+1]/len + 1.0) * 0.5;
                    data[ngen+2] = (data[ngen+2]/len + 1.0) * 0.5;
                    data[ngen+3] = (data[ngen+3]/len + 1.0) * 0.5;
                }
                ngen += 4;
            }
        }
    }

#undef BORDER
}
