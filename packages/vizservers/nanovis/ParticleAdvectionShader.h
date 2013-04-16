/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_PARTICLE_ADVECTION_SHADER_H
#define NV_PARTICLE_ADVECTION_SHADER_H

#include <vrmath/Vector3f.h>

#include "Shader.h"

namespace nv {

class ParticleAdvectionShader : public Shader
{
public:
    ParticleAdvectionShader();

    virtual ~ParticleAdvectionShader();

    virtual void bind(unsigned int texID, unsigned int initPosTexID, bool init);

    virtual void unbind();

    void setScale(const vrmath::Vector3f& scale)
    {
        _scale = scale;
    }

    void setVelocityVolume(unsigned int texID, float max)
    {
        _velocityVolumeID = texID;
        _max = max;
        // FIXME: Is this needed?
        if (_max > 100.f)
            _max = 100.0f;
    }

    void setTimeStep(float timeStep)
    {
        _timeStep = timeStep;
    }

    void setRenderMode(int mode)
    {
        _mode = mode;
    }

private:
    void init();

    unsigned int _velocityVolumeID;
    vrmath::Vector3f _scale;
    float _max;
    float _timeStep;
    int _mode;
};

}

#endif
