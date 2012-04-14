/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

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
}

void NvRegularVolumeShader::bind(unsigned int tfID, Volume *volume, int sliceMode)
{
    //regular cubic volume
    setGLStateMatrixFPParameter("modelViewInv", MODELVIEW_MATRIX,
                                MATRIX_INVERSE);
    setGLStateMatrixFPParameter("modelView", MODELVIEW_MATRIX,
                                MATRIX_IDENTITY);

    setFPTextureParameter("volume", volume->id);
    setFPTextureParameter("tf", tfID);

    if (!sliceMode) {
        setFPParameter4f("renderParameters",
                         volume->numSlices(),
                         volume->opacityScale(),
                         volume->diffuse(),
                         volume->specular());
    } else {
        setFPParameter4f("renderParameters",
                         0.,
                         volume->opacityScale(),
                         volume->diffuse(),
                         volume->specular());
    }

    setFPParameter4f("options",
                     0.0f,
                     volume->isosurface(),
                     0.0f,
                     0.0f);

    NvShader:: bind();
}

void NvRegularVolumeShader::unbind()
{
    disableFPTextureParameter("volume");
    disableFPTextureParameter("tf");

    NvShader::unbind();
}
