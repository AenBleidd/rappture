/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_PARTICLE_ADV_SHADER_H
#define NV_PARTICLE_ADV_SHADER_H

#include "Vector3.h"
#include "NvShader.h"

class NvParticleAdvectionShader : public NvShader
{
public : 
    NvParticleAdvectionShader();

    virtual ~NvParticleAdvectionShader();

    void bind(unsigned int texID, unsigned int initPosTexID);

    void unbind();

    void setScale(const Vector3& scale)
    {
        _scale = scale;
    }

    void setVelocityVolume(unsigned int texID, float max)
    {
        _velocityVolumeID = texID;
        _max = max;
    }

    void setTimeStep(float timeStep)
    {
        _timeStep = timeStep;
    }

    void setRenderMode(int mode)
    {
        _mode = mode;
    }

private :
    void init();

    CGparameter _posTimestepParam;
    CGparameter _velTexParam;
    CGparameter _posTexParam;
    CGparameter _initPosTexParam;
    CGparameter _scaleParam;
    CGparameter _maxParam;
    CGparameter _modeParam;

    unsigned int _velocityVolumeID;
    Vector3 _scale;
    float _max;
    float _timeStep;
    int _mode;
};

#endif
