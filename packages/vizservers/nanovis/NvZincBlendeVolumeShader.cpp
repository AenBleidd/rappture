/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <string.h>

#include <GL/glew.h>
#include <Cg/cgGL.h>

#include "NvZincBlendeVolumeShader.h"

NvZincBlendeVolumeShader::NvZincBlendeVolumeShader()
{
    init();
}

NvZincBlendeVolumeShader::~NvZincBlendeVolumeShader()
{
}

void NvZincBlendeVolumeShader::init()
{
    loadFragmentProgram("zincblende_volume.cg", "main");
    _tfParam = cgGetNamedParameter(_cgFP, "tf");
    _volumeAParam = cgGetNamedParameter(_cgFP, "volumeA");
    _volumeBParam = cgGetNamedParameter(_cgFP, "volumeB");
    _cellSizeParam = cgGetNamedParameter(_cgFP, "cellSize");
    _mviParam = cgGetNamedParameter(_cgFP, "modelViewInv");
    _renderParam = cgGetNamedParameter(_cgFP, "renderParameters");
    _option_one_volume_param = cgGetNamedParameter(_cgFP, "options");
}

void NvZincBlendeVolumeShader::bind(unsigned int tfID, Volume *volume, int sliceMode)
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

void NvZincBlendeVolumeShader::unbind() 
{
    cgGLDisableTextureParameter(_volumeAParam);
    cgGLDisableTextureParameter(_volumeBParam);
    cgGLDisableTextureParameter(_tfParam);

    cgGLDisableProfile(CG_PROFILE_FP30);
}
