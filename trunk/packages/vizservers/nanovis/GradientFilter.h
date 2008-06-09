#ifndef _GRADIENT_FILTER_H
#define _GRADIENT_FILTER_H

typedef enum {DATRAW_UCHAR, DATRAW_FLOAT, DATRAW_USHORT} DataType;

extern void computeGradients(float *gradients, void* volData, int *sizes, DataType dataType);
extern void filterGradients(float *gradients, int *sizes);
extern void quantizeGradients(float *gradientsIn, void *gradientsOut,
					   int *sizes, DataType dataType);

#endif /*_GRADIENT_FILTER_H*/
