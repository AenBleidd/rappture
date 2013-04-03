/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */

#include "NvZincBlendeVolumeShader.h"
#include "ZincBlendeVolume.h"

using namespace nv;

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
}

void NvZincBlendeVolumeShader::bind(unsigned int tfID, Volume *volume,
                                    int sliceMode, float sampleRatio)
{
    ZincBlendeVolume *vol = (ZincBlendeVolume *)volume;
    setGLStateMatrixFPParameter("modelViewInv", MODELVIEW_MATRIX,
                                MATRIX_INVERSE);

    setFPTextureParameter("tf", tfID);
    setFPTextureParameter("volumeA", vol->zincblendeTex[0]->id());
    setFPTextureParameter("volumeB", vol->zincblendeTex[1]->id());

    setFPParameter4f("cellSize",
                     vol->cellSize.x,
                     vol->cellSize.y,
                     vol->cellSize.z, 0.);

    setFPParameter4f("renderParams",
                     (sliceMode ? 0.0f : sampleRatio),
                     volume->isosurface(),
                     volume->opacityScale(),
                     0.0f);

    NvShader::bind();
}

void NvZincBlendeVolumeShader::unbind() 
{
    disableFPTextureParameter("tf");
    disableFPTextureParameter("volumeA");
    disableFPTextureParameter("volumeB");

    NvShader::unbind();
}
