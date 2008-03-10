#ifndef _DX_READER_COMMON_H
#define _DX_READER_COMMON_H


float* merge(float* scalar, float* gradient, int size);
void normalizeScalar(float* fdata, int count, float min, float max);
float* computeGradient(float* fdata, int width, int height, int depth, 
	float min, float max);

#endif /*_DX_READER_COMMON_H*/
