
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "GradientFilter.h"

#ifndef SQR
#define SQR(a) ((a) * (a))
#endif

#define GRADIENTS_EXT		".grd"
int g_numOfSlices[3] = { 256, 256, 256 };
void* g_volData = 0;
float g_sliceDists[3];

#define SOBEL				1
#define GRAD_FILTER_SIZE    5
#define SIGMA2				5.0
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define EPS 1e-5f

static char *getFloatGradientsFilename(void)
{
	char floatExt[] = "_float";
    char *filename;

	/*
    if (! (filename = (char *)malloc(strlen(g.basename) +
									 strlen(floatExt) +
                                     strlen(GRADIENTS_EXT) + 1))) {
										 */
	if (! (filename = (char *)malloc(strlen("base") +
									 strlen(floatExt) +
                                     strlen(GRADIENTS_EXT) + 1))) {
        fprintf(stderr, "not enough memory for filename\n");
        exit(1);
    }

    //strcpy(filename, g.basename);
	strcpy(filename, "base");
	
	strcat(filename, floatExt);
    strcat(filename, GRADIENTS_EXT);

    return filename;
}


static void saveFloatGradients(float *gradients, int *sizes)
{
    char *filename;
    FILE *fp;

    filename = getFloatGradientsFilename();
    if (! (fp = fopen(filename, "wb"))) {
        perror("cannot open gradients file for writing");
        exit(1);
    }

    if (fwrite(gradients, 3 * sizes[0] * sizes[1] * sizes[2] * sizeof(float), 
			   1, fp) != 1) {
        fprintf(stderr, "writing float gradients failed\n");
        exit(1);
    }

    fclose(fp);
}


int getNextPowerOfTwo(int n)
{
	int i;

	i = 1;
	while (i < n) {
		i *= 2;
	}

	return i;
}

static unsigned char getVoxel8(int x, int y, int z)
{
	return ((unsigned char*)g_volData)[z * g_numOfSlices[0] * g_numOfSlices[1] + 
									   y * g_numOfSlices[0] + 
									   x];
}

static unsigned short getVoxel16(int x, int y, int z)
{
	return ((unsigned short*)g_volData)[z * g_numOfSlices[0] * g_numOfSlices[1] + 
										y * g_numOfSlices[0] + 
										x];
}

static unsigned char getVoxelFloat(int x, int y, int z)
{
	return ((float*)g_volData)[z * g_numOfSlices[0] * g_numOfSlices[1] + 
									   y * g_numOfSlices[0] + 
									   x];
}

static float getVoxel(int x, int y, int z, DataType dataType)
{
	switch (dataType) {
		case DATRAW_UCHAR:
			return (float)getVoxel8(x, y, z);
			break;
		case DATRAW_USHORT:
			return (float)getVoxel16(x, y, z);
			break;
		case DATRAW_FLOAT :
			return (float)getVoxelFloat(x, y, z);
			break;
		default:
			fprintf(stderr, "Unsupported data type\n");
			exit(1);
			break;
	}
	return 0.0;
}


void computeGradients(float *gradients, void* volData, int *sizes, DataType dataType)
{
	::g_volData = volData;
	g_numOfSlices[0] = sizes[0];
	g_numOfSlices[1] = sizes[1];
	g_numOfSlices[2] = sizes[2];

	g_sliceDists[0] = 1.0f / sizes[0];
	g_sliceDists[1] = 1.0f / sizes[1];
	g_sliceDists[2] = 1.0f / sizes[2];

    int i, j, k, dir, di, vdi, idz, idy, idx;
    float *gp;

    static int weights[][3][3][3] = {
        {{{-1, -3, -1},
          {-3, -6, -3},
          {-1, -3, -1}},
         {{ 0,  0,  0},
          { 0,  0,  0},
          { 0,  0,  0}},
         {{ 1,  3,  1},
          { 3,  6,  3},
          { 1,  3,  1}}},
        {{{-1, -3, -1},
          { 0,  0,  0},
          { 1,  3,  1}},
         {{-3, -6, -3},
          { 0,  0,  0},
          { 3,  6,  3}},
         {{-1, -3, -1},
          { 0,  0,  0},
          { 1,  3,  1}}},
        {{{-1,  0,  1},
          {-3,  0,  3},
          {-1,  0,  1}},
         {{-3,  0,  3},
          {-6,  0,  6},
          {-3,  0,  3}},
         {{-1,  0,  1},
          {-3,  0,  3},
          {-1,  0,  1}}}
    };

	fprintf(stderr, "computing gradients ... may take a while\n");

    di = 0;
    vdi = 0;
	gp = gradients;
    for (idz = 0; idz < sizes[2]; idz++) {
        for (idy = 0; idy < sizes[1]; idy++) {
            for (idx = 0; idx < sizes[0]; idx++) {
#if SOBEL == 1
                if (idx > 0 && idx < sizes[0] - 1 &&
                    idy > 0 && idy < sizes[1] - 1 &&
                    idz > 0 && idz < sizes[2] - 1) {

                    for (dir = 0; dir < 3; dir++) {
                        gp[dir] = 0.0;
                        for (i = -1; i < 2; i++) {
                            for (j = -1; j < 2; j++) {
                                for (k = -1; k < 2; k++) {
                                    gp[dir] += weights[dir][i + 1]
                                                           [j + 1]
                                                           [k + 1] *
                                               getVoxel(idx + i,
                                                      idy + j,
                                                      idz + k,
													  dataType);
                                }
                            }
                        }

						gp[dir] /= 2.0 * g_sliceDists[dir];
                    }
                } else {
                    /* X-direction */
                    if (idx < 1) {
                        gp[0] = (getVoxel(idx + 1, idy, idz, dataType) -
                                 getVoxel(idx, idy, idz, dataType))/
                                (g_sliceDists[0]);
                    } else {
                        gp[0] = (getVoxel(idx, idy, idz, dataType) -
                                 getVoxel(idx - 1, idy, idz, dataType))/
                                (g_sliceDists[0]);
                    }

                    /* Y-direction */
                    if (idy < 1) {
                        gp[1] = (getVoxel(idx, idy + 1, idz, dataType) -
                                   getVoxel(idx, idy, idz, dataType))/
                                   (g_sliceDists[1]);
                    } else {
                        gp[1] = (getVoxel(idx, idy, idz, dataType) -
                                   getVoxel(idx, idy - 1, idz, dataType))/
                                   (g_sliceDists[1]);
                    }

                    /* Z-direction */
                    if (idz < 1) {
                        gp[2] = (getVoxel(idx, idy, idz + 1, dataType) -
                                 getVoxel(idx, idy, idz, dataType))/
                                (g_sliceDists[2]);
                    } else {
                        gp[2] = (getVoxel(idx, idy, idz, dataType) -
                                 getVoxel(idx, idy, idz - 1, dataType))/
                                (g_sliceDists[2]);
                    }
                }
#else
                /* X-direction */
                if (idx < 1) {
                    gp[0] = (getVoxel(idx + 1, idy, idz, dataType) -
                             getVoxel(idx, idy, idz, dataType))/
                            (g_sliceDists[0]);
                } else if (idx > g_numOfSlices[0] - 1) {
                    gp[0] = (getVoxel(idx, idy, idz, dataType) -
                             getVoxel(idx - 1, idy, idz, dataType))/
                            (g_sliceDists[0]);
                } else {
                    gp[0] = (getVoxel(idx + 1, idy, idz, dataType) -
                             getVoxel(idx - 1, idy, idz, dataType))/
                            (2.0 * g_sliceDists[0]);
                }

                /* Y-direction */
                if (idy < 1) {
                    gp[1] = (getVoxel(idx, idy + 1, idz, dataType) -
                             getVoxel(idx, idy, idz, dataType))/
                            (g_sliceDists[1]);
                } else if (idy > g_numOfSlices[1] - 1) {
                    gp[1] = (getVoxel(idx, idy, idz, dataType) -
                             getVoxel(idx, idy - 1, idz, dataType))/
                            (g_sliceDists[1]);
                } else {
                    gp[1] = (getVoxel(idx, idy + 1, idz, dataType) -
                             getVoxel(idx, idy - 1, idz, dataType))/
                            (2.0 * g_sliceDists[1]);
                }

                /* Z-direction */
                if (idz < 1) {
                    gp[2] = (getVoxel(idx, idy, idz + 1, dataType) -
                             getVoxel(idx, idy, idz, dataType))/
                            (g_sliceDists[2]);
                } else if (idz > g_numOfSlices[2] - 1) {
                    gp[2] = (getVoxel(idx, idy, idz, dataType) -
                             getVoxel(idx, idy, idz - 1, dataType))/
                            (g_sliceDists[2]);
                } else {
                    gp[2] = (getVoxel(idx, idy, idz + 1, dataType) -
                             getVoxel(idx, idy, idz - 1, dataType))/
                            (2.0 * g_sliceDists[2]);
                }
#endif
                gp += 3;
            }
        }
    }

}

void filterGradients(float *gradients, int *sizes)
{
    int i, j, k, idz, idy, idx, gi, ogi, filterWidth, n, borderDist[3];
    float sum, *filteredGradients, ****filter;

	fprintf(stderr, "filtering gradients ... may also take a while\n");

    if (! (filteredGradients = (float *)malloc(sizes[0] * sizes[1] * sizes[2]
											   * 3 * sizeof(float)))) {
        fprintf(stderr, "not enough memory for filtered gradients\n");
        exit(1);
    }

	/* Allocate storage for filter kernels */
	if (! (filter = (float ****)malloc((GRAD_FILTER_SIZE/2 + 1) * 
									   sizeof(float ***)))) {
		fprintf(stderr, "failed to allocate gradient filter\n");
		exit(1);
	}
	
	for (i = 0; i < GRAD_FILTER_SIZE/2 + 1; i++) {
		if (! (filter[i] = (float ***)malloc((GRAD_FILTER_SIZE) * 
											 sizeof(float **)))) {
			fprintf(stderr, "failed to allocate gradient filter\n");
			exit(1);
		}
	}
	for (i = 0; i < GRAD_FILTER_SIZE/2 + 1; i++) {
		for (j = 0; j < GRAD_FILTER_SIZE; j++) {
			if (! (filter[i][j] = (float **)malloc((GRAD_FILTER_SIZE) * 
												   sizeof(float *)))) {
				fprintf(stderr, "failed to allocate gradient filter\n");
				exit(1);
			}
		}
	}
	for (i = 0; i < GRAD_FILTER_SIZE/2 + 1; i++) {
		for (j = 0; j < GRAD_FILTER_SIZE; j++) {
			for (k = 0; k < GRAD_FILTER_SIZE; k++) {
				if (! (filter[i][j][k] = (float *)malloc((GRAD_FILTER_SIZE) *
														 sizeof(float)))) {
					fprintf(stderr, "failed to allocate gradient filter\n");
					exit(1);
				}
			}
		}
	}

    filterWidth = GRAD_FILTER_SIZE/2;
	
	/* Compute the filter kernels */
    for (n = 0; n <= filterWidth; n++) {
        sum = 0.0f;
        for (k = -filterWidth; k <= filterWidth; k++) {
            for (j = -filterWidth; j <= filterWidth; j++) {
                for (i = -filterWidth; i <= filterWidth; i++) {
                    sum += (filter[n][filterWidth + k]
									 [filterWidth + j]
									 [filterWidth + i] =
							exp(-(SQR(i) + SQR(j) + SQR(k)) / SIGMA2));
                }
            }
        }
        for (k = -filterWidth; k <= filterWidth; k++) {
            for (j = -filterWidth; j <= filterWidth; j++) {
                for (i = -filterWidth; i <= filterWidth; i++) {
                    filter[n][filterWidth + k]
							 [filterWidth + j]
							 [filterWidth + i] /= sum;
                }
            }
        }
    }

    gi = 0;
    /* Filter the gradients */
    for (idz = 0; idz < sizes[2]; idz++) {
        for (idy = 0; idy < sizes[1]; idy++) {
            for (idx = 0; idx < sizes[0]; idx++) {
                borderDist[0] = MIN(idx, sizes[0] - idx - 1);
                borderDist[1] = MIN(idy, sizes[1] - idy - 1);
                borderDist[2] = MIN(idz, sizes[2] - idz - 1);

                filterWidth = MIN(GRAD_FILTER_SIZE/2,
                                  MIN(MIN(borderDist[0], 
								  		  borderDist[1]),
										  borderDist[2]));

                for (n = 0; n < 3; n++) {
                    filteredGradients[gi] = 0.0;
                    for (k = -filterWidth; k <= filterWidth; k++) {
                        for (j = -filterWidth; j <= filterWidth; j++) {
                            for (i = -filterWidth; i <= filterWidth; i++) {
                                ogi = (((idz + k) * sizes[1]  + (idy + j)) *
									  sizes[0] + (idx + i)) * 3 + 
									  n;
                                filteredGradients[gi] += filter[filterWidth]
													   [filterWidth + k]
													   [filterWidth + j]
													   [filterWidth + i] * 
													   gradients[ogi];
                            }
                        }
                    }
                    gi++;
                }
            }
        }
    }

	/* Replace the orignal gradients by the filtered gradients */
	memcpy(gradients, filteredGradients, 
		   sizes[0] * sizes[1] * sizes[2] * 3 * sizeof(float));

    free(filteredGradients);

	/* free storage of filter kernel(s) */
	for (i = 0; i < GRAD_FILTER_SIZE/2 + 1; i++) {
		for (j = 0; j < GRAD_FILTER_SIZE; j++) {
			for (k = 0; k < GRAD_FILTER_SIZE; k++) {
				free(filter[i][j][k]);
			}
		}
	}
	for (i = 0; i < GRAD_FILTER_SIZE/2 + 1; i++) {
		for (j = 0; j < GRAD_FILTER_SIZE; j++) {
			free(filter[i][j]);
		}
	}
	for (i = 0; i < GRAD_FILTER_SIZE/2 + 1; i++) {
		free(filter[i]);
	}
	free(filter);
}


void quantize8(float *grad, unsigned char *data)
{
	float len;
	int i;

	len = sqrt(SQR(grad[0]) + SQR(grad[1]) + SQR(grad[2]));

	if (len < EPS) {
		grad[0] = grad[1] = grad[2] = 0.0;
	} else {
		grad[0] /= len;
		grad[1] /= len;
		grad[2] /= len;
	}

	for (i = 0; i < 3; i++) {
		data[i] = (unsigned char)((grad[i] + 1.0)/2.0 * UCHAR_MAX);
	}
}

void quantize16(float *grad, unsigned short *data)
{
	float len;
	int i;

	len = sqrt(SQR(grad[0]) + SQR(grad[1]) + SQR(grad[2]));

	if (len < EPS) {
		grad[0] = grad[1] = grad[2] = 0.0;
	} else {
		grad[0] /= len;
		grad[1] /= len;
		grad[2] /= len;
	}

	for (i = 0; i < 3; i++) {
		data[i] = (unsigned short)((grad[i] + 1.0)/2.0 * USHRT_MAX);
	}
}

void quantizeFloat(float *grad, float *data)
{
	float len;
	int i;

	len = sqrt(SQR(grad[0]) + SQR(grad[1]) + SQR(grad[2]));

	if (len < EPS) {
		grad[0] = grad[1] = grad[2] = 0.0;
	} else {
		grad[0] /= len;
		grad[1] /= len;
		grad[2] /= len;
	}

	for (i = 0; i < 3; i++) {
		data[i] = (float)((grad[i] + 1.0)/2.0);
	}
}

void quantizeGradients(float *gradientsIn, void *gradientsOut,
					   int *sizes, DataType dataType)
{

	int idx, idy, idz, di;
	
    di = 0;
    for (idz = 0; idz < sizes[2]; idz++) {
        for (idy = 0; idy < sizes[1]; idy++) {
            for (idx = 0; idx < sizes[0]; idx++) {
				switch (dataType) {
				case DATRAW_UCHAR:
					quantize8(&gradientsIn[di], 
							  &((unsigned char*)gradientsOut)[di]);
					break;
				case DATRAW_USHORT:
					quantize16(&gradientsIn[di], 
							   &((unsigned short*)gradientsOut)[di]);
					break;
				case DATRAW_FLOAT:
					quantizeFloat(&gradientsIn[di], 
							   &((float*)gradientsOut)[di]);
					break;
				default:
					fprintf(stderr, "unsupported data type\n");
					break;
				}
                di += 3;
            }
        }
    }
}
/*

void saveGradients(void *gradients, int *sizes, DataType dataType)
{
    char *filename;
	int size;
    FILE *fp;

    filename = getGradientsFilename();
    if (! (fp = fopen(filename, "wb"))) {
        perror("cannot open gradients file for writing");
        exit(1);
    }

	size = 3 * sizes[0] * sizes[1] * sizes[2]
		   * getDataTypeSize(dataType);
	
    if (fwrite(gradients, size, 1, fp) != 1) {
        fprintf(stderr, "writing gradients failed\n");
        exit(1);
    }

    fclose(fp);
}
*/
