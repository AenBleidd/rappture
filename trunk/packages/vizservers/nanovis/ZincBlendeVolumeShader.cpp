/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */

#include "ZincBlendeVolumeShader.h"
#include "ZincBlendeVolume.h"

using namespace nv;

ZincBlendeVolumeShader::ZincBlendeVolumeShader()
{
    init();
}

ZincBlendeVolumeShader::~ZincBlendeVolumeShader()
{
}

void ZincBlendeVolumeShader::init()
{
    loadFragmentProgram("zincblende_volume.cg");
}

void ZincBlendeVolumeShader::bind(unsigned int tfID, Volume *volume,
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

    Shader::bind();
}

void ZincBlendeVolumeShader::unbind() 
{
    disableFPTextureParameter("tf");
    disableFPTextureParameter("volumeA");
    disableFPTextureParameter("volumeB");

    Shader::unbind();
}
