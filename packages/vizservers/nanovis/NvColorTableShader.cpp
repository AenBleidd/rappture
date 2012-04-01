/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <R2/R2FilePath.h>

#include <GL/glew.h>
#include <Cg/cgGL.h>

#include "NvColorTableShader.h"
#include "Trace.h"

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
    _dataParam = getNamedParameterFromFP("data");
    _tfParam = getNamedParameterFromFP("tf");
    _renderParam = getNamedParameterFromFP("render_param");
}

void NvColorTableShader::bind(Texture2D *plane, TransferFunction *tf)
{
    cgGLSetTextureParameter(_dataParam, plane->id());
    cgGLSetTextureParameter(_tfParam, tf->id());
    cgGLEnableTextureParameter(_dataParam);
    cgGLEnableTextureParameter(_tfParam);
    cgGLSetParameter4f(_renderParam, 0., 0., 0., 0.);

    NvShader::bind();
}

void NvColorTableShader::unbind()
{
    cgGLDisableTextureParameter(_dataParam);
    cgGLDisableTextureParameter(_tfParam);

    NvShader::unbind();
}
