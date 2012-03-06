/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __NV_PARTICLE_ADV_SHADER_H__
#define __NV_PARTICLE_ADV_SHADER_H__

#include "Vector3.h"
#include "NvShader.h"

class NvParticleAdvectionShader : public NvShader {

    CGparameter _posTimestepParam;
    CGparameter _velTexParam;
    //CGparameter _tfTexParam;
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

public : 
    NvParticleAdvectionShader();
    ~NvParticleAdvectionShader();

private :
    void init();
public :
    //void bind(unsigned int texID, unsigned int tfTexID, unsigned int initPosTexID);
    void bind(unsigned int texID, unsigned int initPosTexID);
    void unbind();
    void setScale(const Vector3& scale);
    void setVelocityVolume(unsigned int texID, float max);
    void setTimeStep(float timeStep);
    void setRenderMode(int mode);
};

inline void NvParticleAdvectionShader::setTimeStep(float timeStep)
{
    _timeStep = timeStep;
}

inline void NvParticleAdvectionShader::unbind()
{
     cgGLDisableProfile(CG_PROFILE_FP30);
   
     cgGLDisableTextureParameter(_velTexParam);
     cgGLDisableTextureParameter(_posTexParam);
     //cgGLDisableTextureParameter(_tfTexParam);
     cgGLDisableTextureParameter(_initPosTexParam);
}

inline void NvParticleAdvectionShader::setScale(const Vector3& scale)
{
    _scale = scale;
}

inline void NvParticleAdvectionShader::setVelocityVolume(unsigned int texID, float max)
{
    _velocityVolumeID = texID;
    _max = max;
}

inline void NvParticleAdvectionShader::setRenderMode(int mode)
{
    _mode = mode;
}

#endif //__NV_PARTICLE_ADV_SHADER_H__
