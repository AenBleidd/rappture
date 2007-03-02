#include "NvZincBlendeReconstructor.h"
#include "ZincBlendeVolume.h"

NvZincBlendeReconstructor* NvZincBlendeReconstructor::_instance = NULL;

NvZincBlendeReconstructor::NvZincBlendeReconstructor()
{
}

NvZincBlendeReconstructor::~NvZincBlendeReconstructor()
{
}

NvZincBlendeReconstructor* NvZincBlendeReconstructor::getInstance()
{
    if (_instance == NULL)
    {
        return (_instance = new NvZincBlendeReconstructor());
    }

    return _instance;
}

ZincBlendeVolume* NvZincBlendeReconstructor::load(const char* fileName)
{
    Vector3 origin, delta;
    int width = 0, height = 0, depth = 0;
    void* data = NULL;

    // TBD
    // INSOO
    // I have to handle the header of the file
    
    return buildUp(origin, delta, width, height, depth, data);
}

struct _NvUnitCellInfo {
    float indexX, indexY, indexZ;
    float atoms[8];

    int getIndex(int width, int height) const 
    {
        // NOTE 
        // Zinc blende data has different axises from OpenGL
        // + z -> +x (OpenGL)
        // + x -> +y (OpenGL)
        // + y -> +z (OpenGL), But in 3D texture coordinate is the opposite direction of z 
        // The reasone why index is multiplied by 4 is that one unit cell has half of eight atoms
        // ,i.e. four atoms are mapped into RGBA component of one texel
        return (indexZ + indexX * width + indexY * width * height) * 4;
    }
};

template<class T>
inline int _NvMax2(T a, T b) { return ((a >= b)? a : b); }

template<class T>
inline int _NvMin2(T a, T b) { return ((a >= b)? a : b); }

template<class T>
inline int _NvMax3(T a, T b, T c) { return ((a >= b)? ((a >= c) ? a : c) : ((b >= c)? b : c)); }

template<class T>
inline int _NvMin3(T a, T b, T c) { return ((a <= b)? ((a <= c) ? a : c) : ((b <= c)? b : c)); }

template<class T>
inline int _NvMax9(T* a, T curMax) { return _NvMax3(_NvMax3(a[0], a[1], a[2]), _NvMax3(a[3], a[4], a[5]), _NvMax3(a[6], a[7], curMax)); }

template<class T>
inline int _NvMin9(T* a, T curMax) { return _NvMin3(_NvMax3(a[0], a[1], a[2]), _NvMin3(a[3], a[4], a[5]), _NvMin3(a[6], a[7], curMax)); }

ZincBlendeVolume* NvZincBlendeReconstructor::buildUp(const Vector3& origin, const Vector3& delta, int width, int height, int depth, void* data)
{
    ZincBlendeVolume* zincBlendeVolume = NULL;

    float *dataVolumeA, *dataVolumeB;
    int cellCount = width * height * depth;
    dataVolumeA = new float[cellCount * sizeof(float) * 4];
    dataVolumeB = new float[cellCount * sizeof(float) * 4];

    _NvUnitCellInfo* srcPtr = (_NvUnitCellInfo*) data;

    float vmin, vmax;
    float* component4A, *component4B;
    int index;

    vmin = vmax = srcPtr->atoms[0];
    for (int i = 0; i < cellCount; ++i)
    {
        index = srcPtr->getIndex(width, height);
        component4A = dataVolumeA + index;
        component4B = dataVolumeB + index;


        component4A[0] = srcPtr->atoms[0];
        component4A[1] = srcPtr->atoms[1];
        component4A[2] = srcPtr->atoms[2];
        component4A[3] = srcPtr->atoms[3];

        component4B[0] = srcPtr->atoms[4];
        component4B[1] = srcPtr->atoms[5];
        component4B[2] = srcPtr->atoms[6];
        component4B[3] = srcPtr->atoms[7];

        vmin = _NvMax9((float*) srcPtr->atoms, vmin);
        vmax = _NvMax9((float*)  srcPtr->atoms, vmax);

        ++srcPtr; 
    }

/*
    zincBlendeVolume = new ZincBlendeVolume(origin.x, origin.y, origin.z, 
		width, height, depth, size, 4, 
		dataVolumeA, dataVolumeB,
        vmin, vmax, cellSize);
*/

    return zincBlendeVolume;
}

