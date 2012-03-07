/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VELOCITY_ARROW_SLICE_H
#define VELOCITY_ARROW_SLICE_H

#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <vector>

#define USE_NANOVIS_LIB

#ifdef USE_NANOVIS_LIB
#include "Texture2D.h"
#include "Vector3.h"
#else
#include <vr3d/vrTexture2D.h>

typedef vrTexture2D Texture2D;

class Vector3
{
public:
    float x, y, z;

    Vector3() :
        x(0.0f), y(0.0f), z(0.0f)
    {}

    Vector3(float x1, float y1, float z1) :
        x(x1), y(y1), z(z1)
    {}

    Vector3 operator*(float scale)
    {
        Vector3 vec;
        vec.x = x * scale;
        vec.y = y * scale;
        vec.z = z * scale;
        return vec;
    }

    Vector3 scale(const Vector3& scale)
    {
        Vector3 vec;
        vec.x = x * scale.x;
        vec.y = y * scale.y;
        vec.z = z * scale.z;
        return vec;
    }

    Vector3 operator*(const Vector3& scale)
    {
        Vector3 vec;
        vec.x = x * scale.x;
        vec.y = y * scale.y;
        vec.z = z * scale.z;
        return vec;
    }

    friend Vector3 operator+(const Vector3& value1, const Vector3& value2);

    void set(float x1, float y1, float z1)
    {
        x = x1; y = y1; z = z1;
    }
};

inline Vector3 operator+(const Vector3& value1, const Vector3& value2)
{
    return Vector3(value1.x + value2.x, value1.y + value2.y, value1.z + value2.z);
}

#endif


class VelocityArrowsSlice
{
public:
    enum RenderMode {
        LINES,
        GLYPHS,
    };

private:
    unsigned int _vectorFieldGraphicsID;
    float _vfXscale;
    float _vfYscale;
    float _vfZscale;
    float _slicePos;
    int _axis;
	
    unsigned int _fbo; 	
    unsigned int _tex;
	
    CGcontext _context;
    CGprogram _queryVelocityFP;
    CGparameter _qvVectorFieldParam;

    int _renderTargetWidth;
    int _renderTargetHeight;
    Vector3 *_velocities;
    std::vector<Vector3> _samplingPositions;
    Vector3 _projectionVector;

    int _tickCountForMinSizeAxis;
    int _tickCountX;
    int _tickCountY;
    int _tickCountZ;
	
    int _pointCount;

    Vector3 _maxVelocityScale;
    Vector3 _arrowColor;

    bool _enabled;	
    bool _dirty;
    bool _dirtySamplingPosition;
    bool _dirtyRenderTarget;

    unsigned int _vertexBufferGraphicsID;

    CGprogram _particleVP;
    CGparameter _mvpParticleParam;
    CGparameter _mvParticleParam;
    CGparameter _mvTanHalfFOVParam;
    CGparameter _mvCurrentTimeParam;
	
    CGprogram _particleFP;
    CGparameter _vectorParticleParam;

    Texture2D *_arrowsTex;

    RenderMode _renderMode;

    void createRenderTarget();
    void computeSamplingTicks();

public:
    VelocityArrowsSlice();
    ~VelocityArrowsSlice();

    void vectorField(unsigned int vfGraphicsID, float xScale, float yScale, float zScale);
    void axis(int axis);
    int axis() const;
    void slicePos(float pos);
    float slicePos() const;
    void queryVelocity();
    void render();
    void enabled(bool enabled)
    {
        _enabled = enabled;
    }
    bool enabled() const
    {
        return _enabled;
    }
    void tickCountForMinSizeAxis(int tickCount);
    int tickCountForMinSizeAxis() const;
    void arrowColor(const Vector3& color);
    void renderMode(RenderMode mode);
    RenderMode renderMode() const;
};

inline int VelocityArrowsSlice::axis() const
{
    return _axis;
}

inline float VelocityArrowsSlice::slicePos() const
{
    return _slicePos;
}


inline int VelocityArrowsSlice::tickCountForMinSizeAxis() const
{
    return _tickCountForMinSizeAxis;
}

inline void VelocityArrowsSlice::arrowColor(const Vector3& color)
{
    _arrowColor = color;
}

inline void VelocityArrowsSlice::renderMode(VelocityArrowsSlice::RenderMode mode)
{
    _renderMode = mode;
    _dirty = true;
}

inline VelocityArrowsSlice::RenderMode VelocityArrowsSlice::renderMode() const
{
    return _renderMode;
}
 
#endif
