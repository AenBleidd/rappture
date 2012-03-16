/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_ZINCBLENDE_SHADER_H
#define NV_ZINCBLENDE_SHADER_H

#include "ZincBlendeVolume.h"
#include "NvVolumeShader.h"

class NvZincBlendeVolumeShader : public NvVolumeShader
{
public :
    NvZincBlendeVolumeShader();

    ~NvZincBlendeVolumeShader();

    void bind(unsigned int tfID, Volume *volume, int sliceMode);

    void unbind();

private :
    void init();

    CGparameter _tfParam;
    CGparameter _volumeAParam;
    CGparameter _volumeBParam;
    CGparameter _cellSizeParam;
    CGparameter _mviParam;
    CGparameter _renderParam;
    CGparameter _option_one_volume_param;
};

inline void NvZincBlendeVolumeShader::bind(unsigned int tfID, Volume *volume, int sliceMode)
{
    ZincBlendeVolume *vol = (ZincBlendeVolume *)volume;
    cgGLSetStateMatrixParameter(_mviParam, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
    cgGLSetTextureParameter(_tfParam, tfID);
    cgGLSetParameter4f(_cellSizeParam,
                       vol->cell_size.x,
                       vol->cell_size.y,
                       vol->cell_size.z, 0.);

    if (!sliceMode) {
        cgGLSetParameter4f(_renderParam,
                           vol->n_slices(),
                           vol->opacity_scale(),
                           vol->diffuse(), 
                           vol->specular());
    } else {
        cgGLSetParameter4f(_renderParam,
                           0.,
                           vol->opacity_scale(),
                           vol->diffuse(),
                           vol->specular());
    }

    cgGLSetParameter4f(_option_one_volume_param,
                       0.0f,
                       volume->isosurface(),
                       0.0f,
                       0.0f);

    cgGLSetTextureParameter(_volumeAParam, vol->zincblende_tex[0]->id());
    cgGLSetTextureParameter(_volumeBParam, vol->zincblende_tex[1]->id());
    cgGLEnableTextureParameter(_volumeAParam);
    cgGLEnableTextureParameter(_volumeBParam);

    cgGLBindProgram(_cgFP);
    cgGLEnableProfile(CG_PROFILE_FP30);
}

inline void NvZincBlendeVolumeShader::unbind() 
{
    cgGLDisableTextureParameter(_volumeAParam);
    cgGLDisableTextureParameter(_volumeBParam);
    cgGLDisableTextureParameter(_tfParam);

    cgGLDisableProfile(CG_PROFILE_FP30);
}

#endif 
