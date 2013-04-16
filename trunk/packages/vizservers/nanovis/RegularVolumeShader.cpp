/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include "RegularVolumeShader.h"

using namespace nv;

RegularVolumeShader::RegularVolumeShader()
{
    init();
}

RegularVolumeShader::~RegularVolumeShader()
{
}

void RegularVolumeShader::init()
{
    loadFragmentProgram("one_volume.cg");
}

void RegularVolumeShader::bind(unsigned int tfID, Volume *volume,
                               int sliceMode, float sampleRatio)
{
    //regular cubic volume
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

    Shader:: bind();
}

void RegularVolumeShader::unbind()
{
    disableFPTextureParameter("volume");
    disableFPTextureParameter("tf");

    Shader::unbind();
}
