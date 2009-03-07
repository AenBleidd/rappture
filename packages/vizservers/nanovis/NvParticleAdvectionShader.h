#ifndef __NV_PARTICLE_ADV_SHADER_H__
#define __NV_PARTICLE_ADV_SHADER_H__

#include "Vector3.h"
#include "NvShader.h"

class NvParticleAdvectionShader : public NvShader {
    CGparameter _posTimestepParam;
    CGparameter _velTexParam;
    CGparameter _posTexParam;
    CGparameter _scaleParam;
    CGparameter _maxParam;
    unsigned int _velocityVolumeID;
    Vector3 _scale;
    float _max;
    float _timeStep;

public : 
    NvParticleAdvectionShader();
    ~NvParticleAdvectionShader();

private :
    void init();
public :
    void bind(unsigned int texID);
    void unbind();
    void setScale(const Vector3& scale);
    void setVelocityVolume(unsigned int texID, float max);
    void setTimeStep(float timeStep);
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

#endif //__NV_PARTICLE_ADV_SHADER_H__
