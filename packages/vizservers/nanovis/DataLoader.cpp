/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include <string>
#include <memory.h>
#include <stdlib.h>
#include <math.h>

#include "DataLoader.h"
#include "Trace.h"

inline void endian_swap(unsigned int& x)
{
    x = (x>>24) | 
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
}

char buff[255];
void getLine(FILE *fp)
{
    char ch;
    int index = 0;
    do {
        ch = fgetc(fp);
        if (ch == '\n') break;
        buff[index++] = ch;
    } while (!feof(fp));

    buff[index] = '\0';
}

void *LoadDx(const char *fname,
             int& width, int& height, int& depth,
             float& min, float& max, float& nonzero_min, 
             float& axisScaleX, float& axisScaleY, float& axisScaleZ)
{
    std::string path = fname;

    if (path.size() == 0) {
        ERROR("file not found[%s]\n", path.c_str());
        return NULL;
    }

    FILE *fp = fopen(path.c_str(), "rb");
    if (fp == NULL) {
        ERROR("file not found %s\n", path.c_str());
        return NULL;
    }

    int index;
    do {
        getLine(fp);
    } while (memcmp(buff, "object", 6));
	
    float origin_x, origin_y, origin_z;
    double delta_x, delta_y, delta_z;
    double temp;
    char str[5][20];

    //object 1 class gridpositions counts 5 5 5
    sscanf(buff, "%s%s%s%s%s%d%d%d", str[0], str[1], str[2], str[3], str[4], &width, &depth, &height);
	
    //origin 0 0 0
    getLine(fp);
    sscanf(buff, "%s%f%f%f", str[0], &(origin_x), &(origin_y), &(origin_z));
	
    // delta  0.5431 0 0
    getLine(fp);
    sscanf(buff, "%s%lf%lf%lf", str[0], &(delta_x), &temp, &temp);
	
    // delta  0 0.5431 0
    getLine(fp);
    sscanf(buff, "%s%lf%lf%lf", str[0], &temp, &(delta_y), &temp);
    
    // delta  0 0 0.5431
    getLine(fp);
    sscanf(buff, "%s%lf%lf%lf", str[0], &temp, &temp, &(delta_z));
	
    getLine(fp);
    getLine(fp);
    getLine(fp);
    getLine(fp);

    min = nonzero_min = 1e27f;
    max = -1e27f;

    float *data = 0;
    int d, h, w;
    float scalar;		
    index = 0;

    if (width < height)	{
        int size = width * height * depth;
        data = (float *) malloc(size * sizeof(float));
        memset(data, 0, sizeof(float) * size);


        for (w = 0; w < width; ++w) {// x
            for (d = 0; d < depth; ++d) { //z
                for (h = 0; h < width; ++h) { // y
                    scalar = atof(fgets(buff, 255, fp));
                    data[h * width + d * width * width + w] = scalar;
                    if (scalar < min) min = scalar;
                    if (scalar != 0.0f && nonzero_min > scalar) nonzero_min = scalar;
                    if (scalar > max) max = scalar;
                }
                for (;h < height; ++h) {
                    scalar = atof(fgets(buff, 255, fp));
                }
            }
        }

        height = width;
    } else if (width > height) {
        int size = width * width * width;
        data = (float *) malloc(size * sizeof(float));
        memset(data, 0, sizeof(float) * size);
        for (w = 0; w < width; ++w) { // x
            for (d = 0; d < depth; ++d) { //z
                for (h = 0; h < height; ++h) { // y
                    scalar = atof(fgets(buff, 255, fp));
                    data[h * width + d * width * width + w] = scalar;
                    if (scalar < min) min = scalar;
                    if (scalar != 0.0f && nonzero_min > scalar) nonzero_min = scalar;
                    if (scalar > max) max = scalar;
                }
            }
        }

        height = width;
    } else {
        int size = width * height * depth;
        data = (float *) malloc(size * sizeof(float));

        for (w = 0; w < width; ++w) { // x
            for (d = 0; d < depth; ++d) { //z
                for (h = 0; h < height; ++h) { // y
                    scalar = atof(fgets(buff, 255, fp));
                    data[h * width + d * width * height + w] = scalar;
                    if (scalar < min) min = scalar;
                    if (scalar != 0.0f && nonzero_min > scalar) nonzero_min = scalar;
                    if (scalar > max) max = scalar;
                }
            }
        }
    }

    axisScaleX = origin_x + width * delta_x;
    axisScaleY = origin_y + height * delta_y;
    axisScaleZ = origin_z + depth * delta_z;

    fclose(fp);

    return data;
}

void *LoadFlowDx(const char *fname,
                 int& width, int& height, int& depth,
                 float& min, float& max, float& nonzero_min, 
                 float& axisScaleX, float& axisScaleY, float& axisScaleZ)
{
    std::string path = fname;

    if (path.size() == 0) {
        ERROR("file not found[%s]\n", path.c_str());
        return NULL;
    }

    FILE *fp = fopen(path.c_str(), "rb");
    if (fp == NULL) {
        ERROR("file not found %s\n", path.c_str());
        return NULL;
    }

    int index;
    do {
        getLine(fp);
    } while (memcmp(buff, "object", 6));
	
    double origin_x, origin_y, origin_z;
    double delta_x, delta_y, delta_z;
    double temp;
    char str[5][20];

    //object 1 class gridpositions counts 5 5 5
    sscanf(buff, "%s%s%s%s%s%d%d%d", str[0], str[1], str[2], str[3], str[4], &width, &height, &depth);
	
    getLine(fp);
    sscanf(buff, "%s%lg%lg%lg", str[0], &(origin_x), &(origin_y), &(origin_z));
	
    getLine(fp);
    sscanf(buff, "%s%lg%lg%lg", str[0], &(delta_x), &temp, &temp);
	
    getLine(fp);
    sscanf(buff, "%s%lg%lg%lg", str[0], &temp, &(delta_y), &temp);
    
    getLine(fp);
    sscanf(buff, "%s%lf%lf%lf", str[0], &temp, &temp, &(delta_z));
	
    getLine(fp);
    getLine(fp);

    min = nonzero_min = 1e27f;
    max = -1e27f;

    float *data = 0;
    int d, h, w;		
    index = 0;

    int size = width * height * depth;
    data = (float *) malloc(size * sizeof(float) * 4);
    float *ptr = data;
    double x, y, z, length;
    char *ret = 0;

    for (w = 0; w < width; ++w) { // x
        for (h = 0; h < height; ++h) { // y	
            for (d = 0; d < depth; ++d) { //z	
                ret = fgets(buff, 255, fp);
                sscanf(buff, "%lg%lg%lg", &x, &y, &z);
                ptr = data + (d * width * height + h * width + w) * 4;
                length = sqrt(x*x+y*y+z*z);
                *ptr = length; ptr++;
                *ptr = x; ptr++;
                *ptr = y; ptr++;
                *ptr = z;

                if (length < min) min = length;
                if (length != 0.0f && nonzero_min > length) nonzero_min = length;
                if (length > max) {
                    //TRACE("max %lf %lf %fl\n", x, y, z);
                    max = length;
                }
            }
        }
    }

    ptr = data;
    for (int i = 0; i < size; ++i) {
        *ptr = *ptr / max; ++ptr;
        *ptr = (*ptr / (2.0*max)) + 0.5; ++ptr;
        *ptr = (*ptr / (2.0 * max)) + 0.5; ++ptr;
        *ptr = (*ptr / (2.0 * max)) + 0.5; ++ptr;
    }
    axisScaleX = width * delta_x;
    axisScaleY = height * delta_y;
    axisScaleZ = depth * delta_z;

    fclose(fp);

    TRACE("width %d, height %d, depth %d, min %f, max %f, scaleX %f, scaleY %f, scaleZ %f\n", 
          width, height, depth, min, max, axisScaleX, axisScaleY, axisScaleZ);
    return data;
}

void *LoadProcessedFlowRaw(const char *fname,
                           int width, int height, int depth,
                           float min, float max,
                           float& axisScaleX, float& axisScaleY, float& axisScaleZ)
{
    std::string path = fname;

    if (fname == 0) {
        ERROR("file name is null\n");
        return NULL;
    }

    FILE *fp = fopen(fname, "rb");
    if (fp == NULL) {
        ERROR("file not found %s\n", fname);
        return NULL;
    }

    int size = width * height * depth;
    float *srcdata = (float *) malloc(size * sizeof(float) * 4);
    fread(srcdata, size * 4 * sizeof(float), 1, fp);
    fclose(fp);

    axisScaleX = width;
    axisScaleY = height;
    axisScaleZ = depth;

    return srcdata;
}

void LoadProcessedFlowRaw(const char *fname,
                          int width, int height, int depth,
                          float *data)
{
    std::string path = fname;

    if (fname == 0) {
        ERROR("file name is null\n");
        return;
    }

    FILE *fp = fopen(fname, "rb");
    if (fp == NULL) {
        ERROR("file not found %s\n", fname);
        return;
    }

    int size = width * height * depth;
    fread(data, size * 4 * sizeof(float), 1, fp);
    fclose(fp);
}

void *LoadFlowRaw(const char *fname, int width, int height, int depth,
                  float& min, float& max, float& nonzero_min, 
                  float& axisScaleX, float& axisScaleY, float& axisScaleZ,
                  int skipByte, bool bigEndian)
{
    std::string path = fname;

    if (path.size() == 0) {
        ERROR("file not found[%s]\n", path.c_str());
        return NULL;
    }

    FILE *fp = fopen(path.c_str(), "rb");
    if (fp == NULL) {
        ERROR("file not found %s\n", path.c_str());
        return NULL;
    }

    min = nonzero_min = 1e27f;
    max = -1e27f;

    float *srcdata = 0;
    float *data = 0;
    int d, h, w;		
    int index = 0;

    int size = width * height * depth;
    srcdata = (float *) malloc(size * sizeof(float) * 3);
    memset(srcdata, 0, size * sizeof(float) * 3);
    if (skipByte != 0) fread(srcdata, skipByte, 1, fp);
    fread(srcdata, size * 3 * sizeof(float), 1, fp);
    fclose(fp);

    if (bigEndian) {
        unsigned int *dd = (unsigned int *) srcdata;
        for (int i = 0; i < size * 3; ++i) {
            endian_swap(dd[i]);
        }
    }

    data = (float *) malloc(size * sizeof(float) * 4);
    memset(data, 0, size * sizeof(float) * 4);

    float *ptr = data;
    double x, y, z, length;

    for (d = 0; d < depth; ++d) { //z	
        for (h = 0; h < height; ++h) { // y	
            for (w = 0; w < width; ++w, index +=3) { // x
                x = srcdata[index];
                y = srcdata[index + 1];
                z = srcdata[index + 2];
                ptr = data + (d * width * height + h * width + w) * 4;
                length = sqrt(x*x+y*y+z*z);
                *ptr = length; ptr++;
                *ptr = x; ptr++;
                *ptr = y; ptr++;
                *ptr = z;

                if (length < min) min = length;
                if (length != 0.0f && nonzero_min > length) nonzero_min = length;
                if (length > max) {
                    TRACE("max %lf %lf %fl\n", x, y, z);
                    max = length;
                }
            }
        }
    }

    ptr = data;
    for (int i = 0; i < size; ++i) {
        *ptr = *ptr / max; ++ptr;
        *ptr = (*ptr / (2.0*max)) + 0.5; ++ptr;
        *ptr = (*ptr / (2.0 * max)) + 0.5; ++ptr;
        *ptr = (*ptr / (2.0 * max)) + 0.5; ++ptr;
    }
    axisScaleX = width;
    axisScaleY = height;
    axisScaleZ = depth;

    fclose(fp);

    TRACE("width %d, height %d, depth %d, min %f, max %f, scaleX %f, scaleY %f, scaleZ %f\n", 
          width, height, depth, min, max, axisScaleX, axisScaleY, axisScaleZ);
    return data;
}
