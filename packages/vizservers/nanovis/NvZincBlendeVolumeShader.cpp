/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

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
}

void NvZincBlendeVolumeShader::bind(unsigned int tfID, Volume *volume, int sliceMode)
{
    ZincBlendeVolume *vol = (ZincBlendeVolume *)volume;
    setGLStateMatrixFPParameter("modelViewInv", MODELVIEW_MATRIX,
                                MATRIX_INVERSE);
    setFPTextureParameter("tf", tfID);
    setFPParameter4f("cellSize",
                     vol->cellSize.x,
                     vol->cellSize.y,
                     vol->cellSize.z, 0.);

    if (!sliceMode) {
        setFPParameter4f("renderParameters",
                         vol->numSlices(),
                         vol->opacityScale(),
                         vol->diffuse(), 
                         vol->specular());
    } else {
        setFPParameter4f("renderParameters",
                         0.,
                         vol->opacityScale(),
                         vol->diffuse(),
                         vol->specular());
    }

    setFPParameter4f("options",
                     0.0f,
                     volume->isosurface(),
                     0.0f,
                     0.0f);

    setFPTextureParameter("volumeA", vol->zincblendeTex[0]->id());
    setFPTextureParameter("volumeB", vol->zincblendeTex[1]->id());

    NvShader::bind();
}

void NvZincBlendeVolumeShader::unbind() 
{
    disableFPTextureParameter("volumeA");
    disableFPTextureParameter("volumeB");
    disableFPTextureParameter("tf");

    NvShader::unbind();
}
