/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  Authors:
 *    Wei Qiao <qiaow@purdue.edu>
 */
#ifndef NV_PARTICLE_RENDERER_H
#define NV_PARTICLE_RENDERER_H

#include <GL/glew.h>

#include <vrmath/Vector3f.h>
#include <vrmath/Color4f.h>

#include "FlowTypes.h"
#include "Volume.h"
#include "ParticleAdvectionShader.h"
#include "RenderVertexArray.h"

namespace nv {

struct Particle {
    float x;
    float y;
    float z;
    float life;

    Particle()
    {}

    Particle(float _x, float _y, float _z, float _life) :
        x(_x),
        y(_y),
        z(_z),
        life(_life)
    {}
};

class ParticleRenderer
{
public:
    ParticleRenderer(int w, int h);

    ~ParticleRenderer();

    void setVectorField(Volume *volume);

    void initialize();

    void advect();

    void reset();

    void render();

    bool active() const
    {
        return _activate;
    }

    void active(bool state)
    {
        _activate = state;
    }

    void setColor(const vrmath::Color4f& color)
    {
        _color = color;
    }

    void setAxis(FlowSliceAxis axis);

    FlowSliceAxis getAxis() const
    {
        return _sliceAxis;
    }

    void setPos(float pos);

    float getPos() const
    {
        return _slicePos;
    }

    void particleSize(float size)
    {
        _particleSize = size;
    }

    float particleSize() const
    {
        return _particleSize;
    }

private:
    void initializeDataArray();

    void updateVertexBuffer();

    ParticleAdvectionShader *_advectionShader;

    /// frame buffer objects: two are defined, flip them as input output every step
    GLuint _psysFbo[2];

    /// color textures attached to frame buffer objects
    GLuint _psysTex[2];
    GLuint _initPosTex;
    Particle *_data;

    /// Count the frame number of particle system iteration
    unsigned int _psysFrame;

    /// Reinitiate particles
    bool _reborn;

    /// flip the source and destination render targets
    bool _flip;

    float _maxLife;

    /// Size of the particle: default is 1.2
    float _particleSize;

    /// vertex array for display particles
    RenderVertexArray *_vertexArray;

    /// scale of flow data
    vrmath::Vector3f _scale;

    vrmath::Vector3f _origin;

    bool _activate;

    float _slicePos;
    FlowSliceAxis _sliceAxis;

    vrmath::Color4f _color;

    //the storage of particles is implemented as a 2D array.
    int _psysWidth;
    int _psysHeight;
};

}

#endif
