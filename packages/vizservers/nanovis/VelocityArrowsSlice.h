/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_VELOCITY_ARROW_SLICE_H
#define NV_VELOCITY_ARROW_SLICE_H

#include <vector>

#include <vrmath/Vector3f.h>

#include "FlowTypes.h"
#include "Volume.h"
#include "Texture2D.h"
#include "Shader.h"

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

    void setVectorField(Volume *volume);

    void setSliceAxis(FlowSliceAxis axis);

    int getSliceAxis() const
    {
        return _axis;
    }

    void setSlicePosition(float pos)
    {
        _slicePos = pos;
        _dirty = true;
    }

    float getSlicePosition() const
    {
        return _slicePos;
    }

    void render();

    void visible(bool visible)
    {
        _visible = visible;
    }

    bool visible() const
    {
        return _visible;
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
    void queryVelocity();

    void createRenderTarget();

    void computeSamplingTicks();

    unsigned int _vectorFieldGraphicsID;
    vrmath::Vector3f _origin;
    vrmath::Vector3f _scale;

    float _slicePos;
    FlowSliceAxis _axis;

    unsigned int _fbo;
    unsigned int _tex;
    float _maxPointSize;

    Shader _queryVelocityFP;
    Shader _particleShader;

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

    bool _visible;
    bool _dirty;
    bool _dirtySamplingPosition;
    bool _dirtyRenderTarget;

    unsigned int _vertexBufferGraphicsID;

    Texture2D *_arrowsTex;

    RenderMode _renderMode;
};

}

#endif
