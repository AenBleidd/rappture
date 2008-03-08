#include "dxReaderCommon.h"
#include "GradientFilter.h"

#include "Vector3.h"
#include "stdlib.h"

/*
float* merge(float* scalar, float* gradient, int size)
{
    float* data = (float*) malloc(sizeof(float) * 4 * size);

    Vector3* g = (Vector3*) gradient;

    int ngen = 0, sindex = 0;
    for (sindex = 0; sindex <size; ++sindex)
    {
        data[ngen++] = scalar[sindex];
        data[ngen++] = g[sindex].x;
        data[ngen++] = g[sindex].y;
        data[ngen++] = g[sindex].z;
    }
    return data;
}

void normalizeScalar(float* fdata, int count, float min, float max)
{
    float v = max - min;
    if (v != 0.0f) 
    {
        for (int i = 0; i < count; ++i)
            fdata[i] = fdata[i] / v;
    }
}

float* computeGradient(float* fdata, int width, int height, int depth, float min, float max)
{
    float* gradients = (float *)malloc(width * height * depth * 3 * sizeof(float));
    float* tempGradients = (float *)malloc(width * height * depth * 3 * sizeof(float));
    int sizes[3] = { width, height, depth };
    computeGradients(tempGradients, fdata, sizes, DATRAW_FLOAT);
    filterGradients(tempGradients, sizes);
    quantizeGradients(tempGradients, gradients, sizes, DATRAW_FLOAT);
    normalizeScalar(fdata, width * height * depth, min, max);
    float* data = merge(fdata, gradients, width * height * depth);

    return data;
}
*/

float *
merge(float* scalar, float* gradient, int size)
{
    float* data = (float*) malloc(sizeof(float) * 4 * size);

    Vector3* g = (Vector3*) gradient;

    int ngen = 0, sindex = 0;
    for (sindex = 0; sindex <size; ++sindex) {
    data[ngen++] = scalar[sindex];
    data[ngen++] = g[sindex].x;
    data[ngen++] = g[sindex].y;
    data[ngen++] = g[sindex].z;
    }
    return data;
}

void
normalizeScalar(float* fdata, int count, float min, float max)
{
    float v = max - min;
    if (v != 0.0f) {
        for (int i = 0; i < count; ++i) {
            fdata[i] = fdata[i] / v;
    }
    }
}

float*
computeGradient(float* fdata, int width, int height, int depth,
        float min, float max)
{
    float* gradients = (float *)malloc(width * height * depth * 3 *
                       sizeof(float));
    float* tempGradients = (float *)malloc(width * height * depth * 3 *
                       sizeof(float));
    int sizes[3] = { width, height, depth };
    computeGradients(tempGradients, fdata, sizes, DATRAW_FLOAT);
    filterGradients(tempGradients, sizes);
    quantizeGradients(tempGradients, gradients, sizes, DATRAW_FLOAT);
    normalizeScalar(fdata, width * height * depth, min, max);
    float* data = merge(fdata, gradients, width * height * depth);
    return data;
}

