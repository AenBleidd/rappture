/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_PARTICLE_ADV_SHADER_H
#define NV_PARTICLE_ADV_SHADER_H

#include "Vector3.h"
#include "NvShader.h"

class NvParticleAdvectionShader : public NvShader
{
public:
    NvParticleAdvectionShader();

    virtual ~NvParticleAdvectionShader();

    virtual void bind(unsigned int texID, unsigned int initPosTexID);

    virtual void unbind();

    void setScale(const Vector3& scale)
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
    Vector3 _scale;
    float _max;
    float _timeStep;
    int _mode;
};

#endif
