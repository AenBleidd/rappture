#include "RpDX.h"

float* merge(float* scalar, float* gradient, int size);
void normalizeScalar(float* fdata, int count, float min, float max);
float* computeGradient(float* fdata, int width, int height, int depth, float min, float max);
