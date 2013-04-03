/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef VELOCITY_ARROW_SLICE_H
#define VELOCITY_ARROW_SLICE_H

#include <vector>

#include <vrmath/Vector3f.h>

#include "Texture2D.h"
#include "NvShader.h"

namespace nv {

class VelocityArrowsSlice
{
public:
    enum RenderMode {
        LINES,
        GLYPHS,
    };

    VelocityArrowsSlice();

    ~VelocityArrowsSlice();

    void setVectorField(unsigned int vfGraphicsID, const vrmath::Vector3f& origin,
                        float xScale, float yScale, float zScale, float max);

    void axis(int axis);

    int axis() const
    {
        return _axis;
    }

    void slicePos(float pos)
    {
        _slicePos = pos;
        _dirty = true;
    }

    float slicePos() const
    {
        return _slicePos;
    }

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

    void tickCountForMinSizeAxis(int tickCount)
    {
        _tickCountForMinSizeAxis = tickCount;
    }

    int tickCountForMinSizeAxis() const
    {
        return _tickCountForMinSizeAxis;
    }

    void arrowColor(const vrmath::Vector3f& color)
    {
        _arrowColor = color;
    }

    void renderMode(RenderMode mode)
    {
        _renderMode = mode;
        _dirty = true;
    }

    RenderMode renderMode() const
    {
        return _renderMode;
    }

private:
    void createRenderTarget();

    void computeSamplingTicks();

    unsigned int _vectorFieldGraphicsID;
    float _vfXscale;
    float _vfYscale;
    float _vfZscale;
    float _slicePos;
    int _axis;

    unsigned int _fbo;
    unsigned int _tex;
    float _maxPointSize;

    NvShader _queryVelocityFP;

    NvShader _particleShader;

    int _renderTargetWidth;
    int _renderTargetHeight;
    vrmath::Vector3f *_velocities;
    std::vector<vrmath::Vector3f> _samplingPositions;
    vrmath::Vector3f _projectionVector;

    int _tickCountForMinSizeAxis;
    int _tickCountX;
    int _tickCountY;
    int _tickCountZ;

    int _pointCount;

    vrmath::Vector3f _maxVelocityScale;
    vrmath::Vector3f _arrowColor;

    bool _enabled;
    bool _dirty;
    bool _dirtySamplingPosition;
    bool _dirtyRenderTarget;

    unsigned int _vertexBufferGraphicsID;

    Texture2D *_arrowsTex;

    RenderMode _renderMode;
};

}

#endif
