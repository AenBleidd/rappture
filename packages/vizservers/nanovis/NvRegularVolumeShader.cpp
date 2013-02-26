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

void NvRegularVolumeShader::bind(unsigned int tfID, Volume *volume,
                                 int sliceMode, float sampleRatio)
{
    //regular cubic volume
    setGLStateMatrixFPParameter("modelViewInv", MODELVIEW_MATRIX,
                                MATRIX_INVERSE);
    setGLStateMatrixFPParameter("modelView", MODELVIEW_MATRIX,
                                MATRIX_IDENTITY);

    setFPTextureParameter("volume", volume->textureID());
    setFPTextureParameter("tf", tfID);

    setFPParameter4f("material",
                     volume->ambient(),
                     volume->diffuse(),
                     volume->specularLevel(),
                     volume->specularExponent());

    setFPParameter4f("renderParams",
                     (sliceMode ? 0.0f : sampleRatio),
                     volume->isosurface(),
                     volume->opacityScale(),
                     (volume->twoSidedLighting() ? 1.0f : 0.0f));

    NvShader:: bind();
}

void NvRegularVolumeShader::unbind()
{
    disableFPTextureParameter("volume");
    disableFPTextureParameter("tf");

    NvShader::unbind();
}
