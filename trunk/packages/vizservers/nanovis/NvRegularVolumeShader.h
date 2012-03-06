/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __NV_REGULAR_SHADER_H__
#define __NV_REGULAR_SHADER_H__

#include "Volume.h"
#include "NvVolumeShader.h"

class NvRegularVolumeShader : public NvVolumeShader {
    CGparameter m_vol_one_volume_param;
    CGparameter m_tf_one_volume_param;
    CGparameter m_mvi_one_volume_param;
    CGparameter m_mv_one_volume_param;
    CGparameter m_render_param_one_volume_param;
    CGparameter m_option_one_volume_param;

public :
    NvRegularVolumeShader();
    ~NvRegularVolumeShader();
private :
    void init();

public :
    void bind(unsigned int tfID, Volume* volume, int sliceMode);
    void unbind();

};

inline void 
NvRegularVolumeShader::bind(unsigned int tfID, Volume* volume, int sliceMode)
{
    //regular cubic volume
    cgGLSetStateMatrixParameter(m_mvi_one_volume_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
    cgGLSetStateMatrixParameter(m_mv_one_volume_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_IDENTITY);

    cgGLSetTextureParameter(m_vol_one_volume_param, volume->id);
    cgGLSetTextureParameter(m_tf_one_volume_param, tfID);
    cgGLEnableTextureParameter(m_vol_one_volume_param);
    cgGLEnableTextureParameter(m_tf_one_volume_param);

    if(!sliceMode)
        cgGLSetParameter4f(m_render_param_one_volume_param,
            volume->n_slices(),
            volume->opacity_scale(),
            volume->diffuse(),
            volume->specular());
    else
        cgGLSetParameter4f(m_render_param_one_volume_param,
            0.,
            volume->opacity_scale(),
            volume->diffuse(),
            volume->specular());

    cgGLSetParameter4f(m_option_one_volume_param,
    	0.0f,
	volume->isosurface(),
	0.0f,
	0.0f);

    cgGLBindProgram(_cgFP);
    cgGLEnableProfile(CG_PROFILE_FP30);
}

inline void NvRegularVolumeShader::unbind()
{
    cgGLDisableTextureParameter(m_vol_one_volume_param);
    cgGLDisableTextureParameter(m_tf_one_volume_param);

    cgGLDisableProfile(CG_PROFILE_FP30);
}


#endif // 
