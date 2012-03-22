/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "dxReaderCommon.h"
#include "GradientFilter.h"

#include "Vector3.h"
#include "stdlib.h"

float *
merge(float *scalar, float *gradient, int size)
{
    float *data = (float *)malloc(sizeof(float) * 4 * size);

    Vector3 *g = (Vector3 *)gradient;

    int ngen = 0, sindex = 0;
    for (sindex = 0; sindex < size; ++sindex) {
        data[ngen++] = scalar[sindex];
        data[ngen++] = g[sindex].x;
        data[ngen++] = g[sindex].y;
        data[ngen++] = g[sindex].z;
    }
    return data;
}

void
normalizeScalar(float *fdata, int count, float min, float max)
{
    float v = max - min;
    if (v != 0.0f) {
        for (int i = 0; i < count; ++i) {
            fdata[i] = fdata[i] / v;
        }
    }
}

float *
computeGradient(float *fdata, int width, int height, int depth,
                float min, float max)
{
    float *gradients = (float *)malloc(width * height * depth * 3 *
                                       sizeof(float));
    float *tempGradients = (float *)malloc(width * height * depth * 3 *
                                           sizeof(float));
    int sizes[3] = { width, height, depth };
    computeGradients(tempGradients, fdata, sizes, DATRAW_FLOAT);
    filterGradients(tempGradients, sizes);
    quantizeGradients(tempGradients, gradients, sizes, DATRAW_FLOAT);
    free(tempGradients);
    normalizeScalar(fdata, width * height * depth, min, max);
    float *data = merge(fdata, gradients, width * height * depth);
    free(gradients);
    return data;
}

void
computeSimpleGradient(float *data, int nx, int ny, int nz)
{
    // Compute the gradient of this data.  BE CAREFUL: center
    // calculation on each node to avoid skew in either direction.
    int ngen = 0;
    for (int iz = 0; iz < nz; iz++) {
        for (int iy = 0; iy < ny; iy++) {
            for (int ix = 0; ix < nx; ix++) {
                // gradient in x-direction
                double valm1 = (ix == 0) ? 0.0 : data[ngen - 4];
                double valp1 = (ix == nx-1) ? 0.0 : data[ngen + 4];
                if (valm1 < 0.0 || valp1 < 0.0) {
                    data[ngen+1] = 0.0;
                } else {
                    data[ngen+1] = valp1-valm1; // assume dx=1
                    //data[ngen+1] = ((valp1-valm1) + 1.0) * 0.5; // assume dx=1 (ISO)
                }

                // gradient in y-direction
                valm1 = (iy == 0) ? 0.0 : data[ngen - 4*nx];
                valp1 = (iy == ny-1) ? 0.0 : data[ngen + 4*nx];
                if (valm1 < 0.0 || valp1 < 0.0) {
                    data[ngen+2] = 0.0;
                } else {
                    data[ngen+2] = valp1-valm1; // assume dy=1
                    //data[ngen+2] = ((valp1-valm1) + 1.0) * 0.5; // assume dy=1 (ISO)
                }

                // gradient in z-direction
                valm1 = (iz == 0) ? 0.0 : data[ngen - 4*nx*ny];
                valp1 = (iz == nz-1) ? 0.0 : data[ngen + 4*nx*ny];
                if (valm1 < 0.0 || valp1 < 0.0) {
                    data[ngen+3] = 0.0;
                } else {
                    data[ngen+3] = valp1-valm1; // assume dz=1
                    //data[ngen+3] = ((valp1-valm1) + 1.0) * 0.5; // assume dz=1 (ISO)
                }

                ngen += 4;
            }
        }
    }
}
