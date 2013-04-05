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

#include "ParticleAdvectionShader.h"
#include "RenderVertexArray.h"

namespace nv {

struct Particle {
    float x;
    float y;
    float z;
    float aux;

    Particle()
    {}

    Particle(float _x, float _y, float _z, float _life) :
        x(_x), y(_y), z(_z), aux(_life)
    {}
};

class ParticleRenderer
{
public:
    ParticleRenderer(int w, int h);

    ~ParticleRenderer();

    void setVectorField(unsigned int texID, const vrmath::Vector3f& origin,
                        float scaleX, float scaleY, float scaleZ, float max);

    void initialize();

    void advect();

    void updateVertexBuffer();

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

    void setAxis(int axis);

    void setPos(float pos);

    void initializeDataArray();

    void particleSize(float size)
    {
        _particleSize = size;
    }

    float particleSize() const
    {
        return _particleSize;
    }

   static ParticleAdvectionShader *_advectionShader;

private:
    /// frame buffer objects: two are defined, flip them as input output every step
    GLuint _psysFbo[2];

    /// color textures attached to frame buffer objects
    GLuint _psysTex[2];
    GLuint _initPosTex;
    Particle *_data;

    /// Count the frame number of particle system iteration
    int _psysFrame;

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
    int _sliceAxis;

    vrmath::Color4f _color;

    //the storage of particles is implemented as a 2D array.
    int _psysWidth;
    int _psysHeight;
};

class ParticleAdvectionShaderInstance
{
public :
    ParticleAdvectionShaderInstance()
    {}

    ~ParticleAdvectionShaderInstance()
    {
        if (ParticleRenderer::_advectionShader) {
            delete ParticleRenderer::_advectionShader;
        }
    }
};

}

#endif
