/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "NvZincBlendeReconstructor.h"
#include "ZincBlendeVolume.h"

NvZincBlendeReconstructor *NvZincBlendeReconstructor::_instance = NULL;

NvZincBlendeReconstructor::NvZincBlendeReconstructor()
{
}

NvZincBlendeReconstructor::~NvZincBlendeReconstructor()
{
}

NvZincBlendeReconstructor *NvZincBlendeReconstructor::getInstance()
{
    if (_instance == NULL) {
        return (_instance = new NvZincBlendeReconstructor());
    }

    return _instance;
}

ZincBlendeVolume *NvZincBlendeReconstructor::loadFromFile(const char *fileName)
{
    Vector3 origin, delta;

    std::ifstream stream;
    stream.open(fileName, std::ios::binary);

    ZincBlendeVolume *volume = loadFromStream(stream);

    stream.close();

    return volume;
}

ZincBlendeVolume *NvZincBlendeReconstructor::loadFromStream(std::istream& stream)
{
    ZincBlendeVolume *volume = NULL;
    Vector3 origin, delta;
    int width = 0, height = 0, depth = 0;
    void *data = NULL;
    int version = 1;

    char str[5][20];
    do {
        getLine(stream);
        if (buff[0] == '#') {
            continue;
        } else if (strstr((const char*) buff, "object") != 0) {
            TRACE("VERSION 1");
            version = 1;
            break;
        } else if (strstr(buff, "record format") != 0) {
            TRACE("VERSION 2");
            version = 2;
            break;
        }
    } while (1);

    if (version == 1) {
	float dummy;

        sscanf(buff, "%s%s%s%s%s%d%d%d", str[0], str[1], str[2], str[3], str[4], &width, &height, &depth);
        getLine(stream); 
        sscanf(buff, "%s%f%f%f", str[0], &(origin.x), &(origin.y), &(origin.z));
        getLine(stream); 
        sscanf(buff, "%s%f%f%f", str[0], &(delta.x), &dummy, &dummy);
        getLine(stream); 
        sscanf(buff, "%s%f%f%f", str[0], &dummy, &(delta.y), &dummy);
        getLine(stream); 
        sscanf(buff, "%s%f%f%f", str[0], &dummy, &dummy, &(delta.z));
        do {
            getLine(stream);
        } while (strcmp(buff, "<\\HDR>") != 0);

        width = width / 4;
        height = height / 4;
        depth = depth / 4;
        //data = new double[width * height * depth * 8 * 4]; 
        data = malloc(width * height * depth * 8 * 4 * sizeof(double)); 
        // 8 atom per cell, 4 double (x, y, z, and probability) per atom
        try {
            stream.read((char *)data, width * height * depth * 8 * 4 * sizeof(double));
        } catch (...) {
            ERROR("Caught exception trying to read stream");
        }

        volume = buildUp(origin, delta, width, height, depth, data);

        free(data);
    } else if (version == 2) {
        const char *pt;
        int datacount;
        double emptyvalue;
        do {
            getLine(stream);
            if ((pt = strstr(buff, "delta")) != 0) {
                sscanf(pt, "%s%f%f%f", str[0], &(delta.x), &(delta.y), &(delta.z));
#ifdef _LOADER_DEBUG_
                TRACE("delta : %f %f %f", delta.x, delta.y, delta.z);
#endif
            } else if ((pt = strstr(buff, "datacount")) != 0) {
                sscanf(pt, "%s%d", str[0], &datacount);
#ifdef _LOADER_DEBUG_
                TRACE("datacount = %d", datacount);
#endif
            } else if ((pt = strstr(buff, "datatype")) != 0) {
                sscanf(pt, "%s%s", str[0], str[1]);
                if (strcmp(str[1], "double64")) {
                }
            } else if ((pt = strstr(buff, "count")) != 0) {
                sscanf(pt, "%s%d%d%d", str[0], &width, &height, &depth);
#ifdef _LOADER_DEBUG_
                TRACE("width height depth %d %d %d", width, height, depth);
#endif
            } else if ((pt = strstr(buff, "emptymark")) != 0) {
                sscanf(pt, "%s%lf", str[0], &emptyvalue);
#ifdef _LOADER_DEBUG_
                TRACE("emptyvalue %lf", emptyvalue);
#endif
            } else if ((pt = strstr(buff, "emprymark")) != 0) {
                sscanf(pt, "%s%lf", str[0], &emptyvalue);
#ifdef _LOADER_DEBUG_
                TRACE("emptyvalue %lf", emptyvalue);
#endif
            }
        } while (strcmp(buff, "<\\HDR>") != 0 && strcmp(buff, "</HDR>") != 0);

        data = malloc(width * height * depth * 8 * 4 * sizeof(double)); 
        memset(data, 0, width * height * depth * 8 * 4 * sizeof(double)); 
        stream.read((char *) data, width * height * depth * 8 * 4 * sizeof(double));

        volume =  buildUp(origin, delta, width, height, depth, datacount, emptyvalue, data);
        free(data);
    }
    return volume;
}

struct _NvAtomInfo {
    double indexX, indexY, indexZ;
    double atom;

    int getIndex(int width, int height) const 
    {
        // NOTE 
        // Zinc blende data has different axes from OpenGL
        // + z -> +x (OpenGL)
        // + x -> +y (OpenGL)
        // + y -> +z (OpenGL), But in 3D texture coordinate is the opposite direction of z 
        // The reason why index is multiplied by 4 is that one unit cell has half of eight atoms,
        // i.e. four atoms are mapped into RGBA component of one texel
        //return ((int) (indexZ - 1)+ (int) (indexX - 1) * width + (int) (indexY - 1) * width * height) * 4;
        return ((int)(indexX - 1) + (int)(indexY - 1) * width + (int)(indexZ - 1) * width * height) * 4;
    }
};

template<class T>
inline T _NvMax2(T a, T b)
{ return ((a >= b)? a : b); }

template<class T>
inline T _NvMin2(T a, T b)
{ return ((a >= b)? a : b); }

template<class T>
inline T _NvMax3(T a, T b, T c)
{ return ((a >= b)? ((a >= c) ? a : c) : ((b >= c)? b : c)); }

template<class T>
inline T _NvMin3(T a, T b, T c)
{ return ((a <= b)? ((a <= c) ? a : c) : ((b <= c)? b : c)); }

template<class T>
inline T _NvMax9(T* a, T curMax)
{ return _NvMax3(_NvMax3(a[0], a[1], a[2]), _NvMax3(a[3], a[4], a[5]), _NvMax3(a[6], a[7], curMax)); }

template<class T>
inline T _NvMin9(T* a, T curMax)
{ return _NvMin3(_NvMax3(a[0], a[1], a[2]), _NvMin3(a[3], a[4], a[5]), _NvMin3(a[6], a[7], curMax)); }

template<class T>
inline T _NvMax4(T* a)
{ return _NvMax2(_NvMax2(a[0], a[1]), _NvMax2(a[2], a[3])); }

template<class T>
inline T _NvMin4(T* a)
{ return _NvMin2(_NvMin2(a[0], a[1]), _NvMin2(a[2], a[3])); }

ZincBlendeVolume * 
NvZincBlendeReconstructor::buildUp(const Vector3& origin, const Vector3& delta,
                                   int width, int height, int depth, void *data)
{
    ZincBlendeVolume *zincBlendeVolume = NULL;

    float *fourAnionVolume, *fourCationVolume;
    int cellCount = width * height * depth;
    fourAnionVolume = new float[cellCount * sizeof(float) * 4];
    fourCationVolume = new float[cellCount * sizeof(float) * 4];

    _NvAtomInfo *srcPtr = (_NvAtomInfo *)data;

    float vmin, vmax, nzero_min;
    float *component4A, *component4B;
    int index;

    nzero_min = 0.0f;		/* Suppress compiler warning. */
    vmin = vmax = srcPtr->atom;

    for (int i = 0; i < cellCount; ++i) {
        index = srcPtr->getIndex(width, height);

#ifdef _LOADER_DEBUG_
        TRACE("index %d", index);
#endif

        component4A = fourAnionVolume + index;
        component4B = fourCationVolume + index;

        component4A[0] = (float)srcPtr->atom; srcPtr++;
        component4A[1] = (float)srcPtr->atom; srcPtr++;
        component4A[2] = (float)srcPtr->atom; srcPtr++;
        component4A[3] = (float)srcPtr->atom; srcPtr++;
      
        component4B[0] = (float)srcPtr->atom; srcPtr++;
        component4B[1] = (float)srcPtr->atom; srcPtr++;
        component4B[2] = (float)srcPtr->atom; srcPtr++;
        component4B[3] = (float)srcPtr->atom; srcPtr++;

        vmax = _NvMax3(_NvMax4(component4A), _NvMax4(component4B), vmax);
        vmin = _NvMin3(_NvMin4(component4A), _NvMin4(component4B), vmin);

        if (vmin != 0.0 && vmin < nzero_min) {
            nzero_min = vmin;
        }
    }

    double dv = vmax - vmin;
    if (vmax != 0.0f) {
        for (int i = 0; i < cellCount; ++i) {
            fourAnionVolume[i] = (fourAnionVolume[i] - vmin)/ dv;
            fourCationVolume[i] = (fourCationVolume[i] - vmin) / dv;
        }
    }

    Vector3 cellSize;
    cellSize.x = 0.25 / width;
    cellSize.y = 0.25 / height;
    cellSize.z = 0.25 / depth;

    zincBlendeVolume = new ZincBlendeVolume(origin.x, origin.y, origin.z,
                                            width, height, depth, 4,
                                            fourAnionVolume, fourCationVolume,
                                            vmin, vmax, nzero_min, cellSize);

    return zincBlendeVolume;
}

ZincBlendeVolume *
NvZincBlendeReconstructor::buildUp(const Vector3& origin, const Vector3& delta,
                                   int width, int height, int depth,
                                   int datacount, double emptyvalue, void* data)
{
    ZincBlendeVolume *zincBlendeVolume = NULL;
    float *fourAnionVolume, *fourCationVolume;
    int size = width * height * depth * 4;
    fourAnionVolume = new float[size];
    fourCationVolume = new float[size];

    memset(fourAnionVolume, 0, size * sizeof(float));
    memset(fourCationVolume, 0, size * sizeof(float));

    _NvAtomInfo *srcPtr = (_NvAtomInfo *) data;

    float *component4A, *component4B;
    float vmin, vmax, nzero_min;
    int index;
    nzero_min = 1e23f;
    vmin = vmax = srcPtr->atom;
    for (int i = 0; i < datacount; ++i) {
        index = srcPtr->getIndex(width, height);

#ifdef _LOADER_DEBUG_
        TRACE("[%d] index %d (width:%lf height:%lf depth:%lf)", i, index, srcPtr->indexX, srcPtr->indexY, srcPtr->indexZ);
#endif

        if (index < 0) {
#ifdef _LOADER_DEBUG_
            TRACE("There is an invalid data");
#endif
            srcPtr +=8;
            continue;
        }

        component4A = fourAnionVolume + index;
        component4B = fourCationVolume + index;

        component4A[0] = (srcPtr->atom != emptyvalue)? (float) srcPtr->atom : 0.0f; srcPtr++;
        component4A[1] = (srcPtr->atom != emptyvalue)? (float) srcPtr->atom : 0.0f; srcPtr++;
        component4A[2] = (srcPtr->atom != emptyvalue)? (float) srcPtr->atom : 0.0f; srcPtr++;
        component4A[3] = (srcPtr->atom != emptyvalue)? (float) srcPtr->atom : 0.0f; srcPtr++;
      
        component4B[0] = (srcPtr->atom != emptyvalue)? (float) srcPtr->atom : 0.0f; srcPtr++;
        component4B[1] = (srcPtr->atom != emptyvalue)? (float) srcPtr->atom : 0.0f; srcPtr++;
        component4B[2] = (srcPtr->atom != emptyvalue)? (float) srcPtr->atom : 0.0f; srcPtr++;
        component4B[3] = (srcPtr->atom != emptyvalue)? (float) srcPtr->atom : 0.0f; srcPtr++;

        vmax = _NvMax3(_NvMax4(component4A), _NvMax4(component4B), vmax);
        vmin = _NvMin3(_NvMin4(component4A), _NvMin4(component4B), vmin);

        if (vmin != 0.0 && vmin < nzero_min) {
            nzero_min = vmin;    
        }
    }

    double dv = vmax - vmin;
    if (vmax != 0.0f) {
        for (int i = 0; i < datacount; ++i) {
            fourAnionVolume[i] = (fourAnionVolume[i] - vmin)/ dv;
            fourCationVolume[i] = (fourCationVolume[i] - vmin) / dv;
        }
    }

    Vector3 cellSize;
    cellSize.x = 0.25 / width;
    cellSize.y = 0.25 / height;
    cellSize.z = 0.25 / depth;

    zincBlendeVolume = new ZincBlendeVolume(origin.x, origin.y, origin.z,
                                            width, height, depth, 4,
                                            fourAnionVolume, fourCationVolume,
                                            vmin, vmax, nzero_min, cellSize);

    return zincBlendeVolume;
}

void NvZincBlendeReconstructor::getLine(std::istream& sin)
{
    char ch;
    int index = 0;
    do {
        sin.get(ch);
        if (ch == '\n') break;
        buff[index++] = ch;
        if (ch == '>') {
            if (buff[1] == '\\')
                break;
        }
    } while (!sin.eof());

    buff[index] = '\0';

#ifdef _LOADER_DEBUG_
    TRACE("%s", buff);
#endif
}

ZincBlendeVolume *
NvZincBlendeReconstructor::loadFromMemory(void *dataBlock)
{
    ZincBlendeVolume *volume = NULL;
    Vector3 origin, delta;
    int width = 0, height = 0, depth = 0;
    void *data = NULL;
    int version = 1;

    unsigned char *stream = (unsigned char *)dataBlock;
    char str[5][20];
    do {
        getLine(stream);
        if (buff[0] == '#') {
            continue;
        } else if (strstr((const char *)buff, "object") != 0) {
            TRACE("VERSION 1");
            version = 1; 
            break;
        } else if (strstr(buff, "record format") != 0) {
            TRACE("VERSION 2");
            version = 2; 
            break;
        }
    } while (1);

    if (version == 1) {
	float dummy;

        sscanf(buff, "%s%s%s%s%s%d%d%d", str[0], str[1], str[2], str[3], str[4],&width, &height, &depth);
        getLine(stream); 
        sscanf(buff, "%s%f%f%f", str[0], &(origin.x), &(origin.y), &(origin.z));
        getLine(stream); 
        sscanf(buff, "%s%f%f%f", str[0], &(delta.x), &dummy, &dummy);
        getLine(stream); 
        sscanf(buff, "%s%f%f%f", str[0], &dummy, &(delta.y), &dummy);
        getLine(stream); 
        sscanf(buff, "%s%f%f%f", str[0], &dummy, &dummy, &(delta.z));
        do {
            getLine(stream);
        } while (strcmp(buff, "<\\HDR>") != 0);

        width = width / 4;
        height = height / 4;
        depth = depth / 4;
        data = malloc(width * height * depth * 8 * 4 * sizeof(double)); 
        // 8 atom per cell, 4 double (x, y, z, and probability) per atom
        memcpy(data, stream, width * height * depth * 8 * 4 * sizeof(double));

        volume = buildUp(origin, delta, width, height, depth, data);

        free(data);
    } else if (version == 2) {
        const char *pt;
        int datacount = -1;
        double emptyvalue;
        do {
            getLine(stream);
            if ((pt = strstr(buff, "delta")) != 0) {   
                sscanf(pt, "%s%f%f%f", str[0], &(delta.x), &(delta.y), &(delta.z));
#ifdef _LOADER_DEBUG_
                TRACE("delta : %f %f %f", delta.x, delta.y, delta.z);
#endif
            } else if ((pt = strstr(buff, "datacount")) != 0) {
                sscanf(pt, "%s%d", str[0], &datacount);
                TRACE("datacount = %d", datacount);
            } else if ((pt = strstr(buff, "datatype")) != 0) {
                sscanf(pt, "%s%s", str[0], str[1]);
                if (strcmp(str[1], "double64")) {
                }
            } else if ((pt = strstr(buff, "count")) != 0) {
                sscanf(pt, "%s%d%d%d", str[0], &width, &height, &depth);
#ifdef _LOADER_DEBUG_
                TRACE("width height depth %d %d %d", width, height, depth);
#endif
            } else if ((pt = strstr(buff, "emptymark")) != 0) {
                sscanf(pt, "%s%lf", str[0], &emptyvalue);
#ifdef _LOADER_DEBUG_
                TRACE("emptyvalue %lf", emptyvalue);
#endif
            } else if ((pt = strstr(buff, "emprymark")) != 0) {
                sscanf(pt, "%s%lf", str[0], &emptyvalue);
#ifdef _LOADER_DEBUG_
                TRACE("emptyvalue %lf", emptyvalue);
#endif
            }
        } while (strcmp(buff, "<\\HDR>") != 0 && strcmp(buff, "</HDR>") != 0);

        if (datacount == -1) datacount = width * height * depth;

        data = malloc(datacount * 8 * 4 * sizeof(double)); 
        memset(data, 0, datacount * 8 * 4 * sizeof(double)); 
        memcpy(data, stream, datacount * 8 * 4 * sizeof(double));

        volume =  buildUp(origin, delta, width, height, depth, datacount, emptyvalue, data);

        free(data);
    }
    return volume;
}

void NvZincBlendeReconstructor::getLine(unsigned char*& stream)
{
    char ch;
    int index = 0;
    do {
        ch = stream[0];
        ++stream;
        if (ch == '\n') break;
        buff[index++] = ch;
        if (ch == '>') {
            if (buff[1] == '\\')
                break;
        }
    } while (1);

    buff[index] = '\0';

#ifdef _LOADER_DEBUG_
    TRACE("%s", buff);
#endif
}
