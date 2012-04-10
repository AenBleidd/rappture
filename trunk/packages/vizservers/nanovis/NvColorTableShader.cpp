/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "NvColorTableShader.h"

NvColorTableShader::NvColorTableShader()
{
    init();
}

NvColorTableShader::~NvColorTableShader()
{
}

void NvColorTableShader::init()
{
    loadFragmentProgram("one_plane.cg", "main"); 
}

void NvColorTableShader::bind(Texture2D *plane, TransferFunction *tf)
{
    setFPTextureParameter("data", plane->id());
    setFPTextureParameter("tf", tf->id());

    setFPParameter4f("render_param", 0., 0., 0., 0.);

    NvShader::bind();
}

void NvColorTableShader::unbind()
{
    disableFPTextureParameter("data");
    disableFPTextureParameter("tf");

    NvShader::unbind();
}
