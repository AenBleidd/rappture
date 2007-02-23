#ifndef __NV_PARTICLE_ADV_SHADER_H__
#define __NV_PARTICLE_ADV_SHADER_H__

#include "Vector3.h"
#include "NvShader.h"

class NvParticleAdvectionShader : public NvShader {
    CGparameter _posTimestepParam;
    CGparameter _velTexParam;
    CGparameter _posTexParam;
    CGparameter _scaleParam;

public : 
    NvParticleAdvectionShader(const Vector3& scale);
    ~NvParticleAdvectionShader();

private :
    void init(const Vector3& scale);

public :
    void bind(unsigned int texID);
    void unbind();
};

inline void NvParticleAdvectionShader::bind(unsigned int texID)
{
    cgGLBindProgram(_cgFP);
    cgGLSetParameter1f(_posTimestepParam, 0.05);
    cgGLEnableTextureParameter(_velTexParam);
    cgGLSetTextureParameter(_posTexParam, texID);
    cgGLEnableTextureParameter(_posTexParam);

    cgGLEnableProfile(CG_PROFILE_FP30);
}

inline void NvParticleAdvectionShader::unbind()
{
     cgGLDisableProfile(CG_PROFILE_FP30);
   
     cgGLDisableTextureParameter(_velTexParam);
     cgGLDisableTextureParameter(_posTexParam);
}

#endif //__NV_PARTICLE_ADV_SHADER_H__
