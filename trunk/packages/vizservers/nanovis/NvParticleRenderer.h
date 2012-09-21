/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * NvParticleRenderer.h: particle system class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef NVPARTICLERENDERER_H
#define NVPARTICLERENDERER_H

#include <GL/glew.h>

#include "NvParticleAdvectionShader.h"
#include "RenderVertexArray.h"
#include "Vector3.h"
#include "Vector4.h"

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

class NvParticleRenderer
{
public:
    NvParticleRenderer(int w, int h);

    ~NvParticleRenderer();

    void setVectorField(unsigned int texID, const Vector3& origin,
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

    void setColor(const Vector4& color)
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

   static NvParticleAdvectionShader *_advectionShader;

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
    Vector3 _scale;

    Vector3 _origin;

    bool _activate;

    float _slicePos;
    int _sliceAxis;

    Vector4 _color;

    //the storage of particles is implemented as a 2D array.
    int _psysWidth;
    int _psysHeight;
};

#endif
