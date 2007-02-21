#ifndef __NV_VOLQD_VOLUME_SHADER_H__
#define __NV_VOLQD_VOLUME_SHADER_H__

#include "Nv.h"
#include "NvVolQDVolume.h"
#include "NvVolumeShader.h"

class NvVolQDVolumeShader : public NvVolumeShader {
    CGparameter _tfParam;
    CGparameter _volumeParam;
    CGparameter _cellSizeParam;
    CGparameter _mviParam;
    CGparameter _renderParam;

public :
    NvVolQDVolumeShader();
    ~NvVolQDVolumeShader();
private :
    void init();
public :
    void bind(unsigned int tfID, Volume* volume, int sliceMode);
    void unbind();

};

inline void NvVolQDVolumeShader::bind(unsigned int tfID, Volume* volume, int sliceMode)
{
    NvVolQDVolume* vol = (NvVolQDVolume*)volume;
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

    cgGLSetTextureParameter(_volumeParam, volume->id);
    cgGLEnableTextureParameter(_volumeParam);

    cgGLBindProgram(_cgFP);
    cgGLEnableProfile(CG_PROFILE_FP30);
}

inline void NvVolQDVolumeShader::unbind()
{
    cgGLDisableTextureParameter(_volumeParam);
    cgGLDisableTextureParameter(_tfParam);

    cgGLDisableProfile(CG_PROFILE_FP30);
}


#endif 
