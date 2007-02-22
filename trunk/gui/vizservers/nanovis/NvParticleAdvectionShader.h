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
    void bind();
    void unbind();
};

#endif //__NV_PARTICLE_ADV_SHADER_H__
