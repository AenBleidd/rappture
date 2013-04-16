/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include "ColorTableShader.h"

using namespace nv;

ColorTableShader::ColorTableShader()
{
    init();
}

ColorTableShader::~ColorTableShader()
{
}

void ColorTableShader::init()
{
    loadFragmentProgram("one_plane.cg"); 
}

void ColorTableShader::bind(Texture2D *plane, TransferFunction *tf)
{
    setFPTextureParameter("data", plane->id());
    setFPTextureParameter("tf", tf->id());

    setFPParameter4f("render_param", 0., 0., 0., 0.);

    Shader::bind();
}

void ColorTableShader::unbind()
{
    disableFPTextureParameter("data");
    disableFPTextureParameter("tf");

    Shader::unbind();
}
