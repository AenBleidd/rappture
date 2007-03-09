#ifndef __NV_ZINCBLENDE_SHADER_H__
#define __NV_ZINCBLENDE_SHADER_H__

#include "Nv.h"
#include "ZincBlendeVolume.h"
#include "NvVolumeShader.h"

class NvZincBlendeVolumeShader : public NvVolumeShader {
    CGparameter _tfParam;
    CGparameter _volumeAParam;
    CGparameter _volumeBParam;
    CGparameter _cellSizeParam;
    CGparameter _mviParam;
    CGparameter _renderParam;

private :
    void init();
public :
    NvZincBlendeVolumeShader();
    ~NvZincBlendeVolumeShader();

    void bind(unsigned int tfID, Volume* volume, int sliceMode);
    void unbind();
};

inline void NvZincBlendeVolumeShader::bind(unsigned int tfID, Volume* volume, int sliceMode)
{
    ZincBlendeVolume* vol = (ZincBlendeVolume*)volume;
    cgGLSetStateMatrixParameter(_mviParam, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
    cgGLSetTextureParameter(_tfParam, tfID);
    cgGLSetParameter4f(_cellSizeParam, vol->cell_size.x, vol->cell_size.y, vol->cell_size.z, 0.);

    if(!sliceMode)
        cgGLSetParameter4f(_renderParam,
            vol->get_n_slice(),
            vol->get_opacity_scale(),
            vol->get_diffuse(), 
            vol->get_specular());
    else
        cgGLSetParameter4f(_renderParam,
            0.,
            vol->get_opacity_scale(),
            vol->get_diffuse(),
            vol->get_specular());

    cgGLSetTextureParameter(_volumeAParam,  vol->zincblende_tex[0]->id);
    cgGLSetTextureParameter(_volumeBParam, vol->zincblende_tex[1]->id);
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
