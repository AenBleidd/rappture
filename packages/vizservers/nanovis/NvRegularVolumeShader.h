/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_REGULAR_SHADER_H
#define NV_REGULAR_SHADER_H

#include "Volume.h"
#include "NvVolumeShader.h"

class NvRegularVolumeShader : public NvVolumeShader
{
public:
    NvRegularVolumeShader();
    ~NvRegularVolumeShader();

    void bind(unsigned int tfID, Volume* volume, int sliceMode);
    void unbind();

private:
    void init();

    CGparameter _vol_one_volume_param;
    CGparameter _tf_one_volume_param;
    CGparameter _mvi_one_volume_param;
    CGparameter _mv_one_volume_param;
    CGparameter _render_param_one_volume_param;
    CGparameter _option_one_volume_param;
};

inline void 
NvRegularVolumeShader::bind(unsigned int tfID, Volume *volume, int sliceMode)
{
    //regular cubic volume
    cgGLSetStateMatrixParameter(_mvi_one_volume_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
    cgGLSetStateMatrixParameter(_mv_one_volume_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_IDENTITY);

    cgGLSetTextureParameter(_vol_one_volume_param, volume->id);
    cgGLSetTextureParameter(_tf_one_volume_param, tfID);
    cgGLEnableTextureParameter(_vol_one_volume_param);
    cgGLEnableTextureParameter(_tf_one_volume_param);

    if (!sliceMode) {
        cgGLSetParameter4f(_render_param_one_volume_param,
            volume->n_slices(),
            volume->opacity_scale(),
            volume->diffuse(),
            volume->specular());
    } else {
        cgGLSetParameter4f(_render_param_one_volume_param,
            0.,
            volume->opacity_scale(),
            volume->diffuse(),
            volume->specular());
    }

    cgGLSetParameter4f(_option_one_volume_param,
    	0.0f,
	volume->isosurface(),
	0.0f,
	0.0f);

    cgGLBindProgram(_cgFP);
    cgGLEnableProfile(CG_PROFILE_FP30);
}

inline void NvRegularVolumeShader::unbind()
{
    cgGLDisableTextureParameter(_vol_one_volume_param);
    cgGLDisableTextureParameter(_tf_one_volume_param);

    cgGLDisableProfile(CG_PROFILE_FP30);
}

#endif
