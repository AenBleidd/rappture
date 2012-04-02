/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <GL/glew.h>
#include <Cg/cgGL.h>

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
    loadFragmentProgram("one_volume.cg", "main");
    _mviOneVolumeParam = getNamedParameterFromFP("modelViewInv");
    _mvOneVolumeParam = getNamedParameterFromFP("modelView");

    _volOneVolumeParam = getNamedParameterFromFP("volume");
    _tfOneVolumeParam = getNamedParameterFromFP("tf");
    _renderParamOneVolumeParam = getNamedParameterFromFP("renderParameters");
    _optionOneVolumeParam = getNamedParameterFromFP("options");
}

void NvRegularVolumeShader::bind(unsigned int tfID, Volume *volume, int sliceMode)
{
    //regular cubic volume
    cgGLSetStateMatrixParameter(_mviOneVolumeParam, CG_GL_MODELVIEW_MATRIX,
                                CG_GL_MATRIX_INVERSE);
    cgGLSetStateMatrixParameter(_mvOneVolumeParam, CG_GL_MODELVIEW_MATRIX,
                                CG_GL_MATRIX_IDENTITY);

    cgGLSetTextureParameter(_volOneVolumeParam, volume->id);
    cgGLSetTextureParameter(_tfOneVolumeParam, tfID);
    cgGLEnableTextureParameter(_volOneVolumeParam);
    cgGLEnableTextureParameter(_tfOneVolumeParam);

    if (!sliceMode) {
        cgGLSetParameter4f(_renderParamOneVolumeParam,
                           volume->numSlices(),
                           volume->opacityScale(),
                           volume->diffuse(),
                           volume->specular());
    } else {
        cgGLSetParameter4f(_renderParamOneVolumeParam,
                           0.,
                           volume->opacityScale(),
                           volume->diffuse(),
                           volume->specular());
    }

    cgGLSetParameter4f(_optionOneVolumeParam,
                       0.0f,
                       volume->isosurface(),
                       0.0f,
                       0.0f);

    NvShader:: bind();
}

void NvRegularVolumeShader::unbind()
{
    cgGLDisableTextureParameter(_volOneVolumeParam);
    cgGLDisableTextureParameter(_tfOneVolumeParam);

    NvShader::unbind();
}
