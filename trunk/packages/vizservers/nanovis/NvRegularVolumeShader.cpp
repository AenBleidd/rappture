/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "global.h"
#include "NvRegularVolumeShader.h"

NvRegularVolumeShader::NvRegularVolumeShader()
{
    init();
}

NvRegularVolumeShader::~NvRegularVolumeShader()
{
}

void NvRegularVolumeShader::init()
{
    _cgFP = LoadCgSourceProgram(g_context, "one_volume.cg", CG_PROFILE_FP30, 
                                "main");
    _mvi_one_volume_param = cgGetNamedParameter(_cgFP, "modelViewInv");
    _mv_one_volume_param = cgGetNamedParameter(_cgFP, "modelView");

    _vol_one_volume_param = cgGetNamedParameter(_cgFP, "volume");
    _tf_one_volume_param = cgGetNamedParameter(_cgFP, "tf");
    _render_param_one_volume_param = cgGetNamedParameter(_cgFP, "renderParameters");
    _option_one_volume_param = cgGetNamedParameter(_cgFP, "options");
}

void NvRegularVolumeShader::bind(unsigned int tfID, Volume *volume, int sliceMode)
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

void NvRegularVolumeShader::unbind()
{
    cgGLDisableTextureParameter(_vol_one_volume_param);
    cgGLDisableTextureParameter(_tf_one_volume_param);

    cgGLDisableProfile(CG_PROFILE_FP30);
}
