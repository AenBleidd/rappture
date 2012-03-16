/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <R2/R2FilePath.h>

#include "NvColorTableShader.h"
#include "Trace.h"
#include "global.h"

NvColorTableShader::NvColorTableShader()
{
    init();
}

NvColorTableShader::~NvColorTableShader()
{
}

void NvColorTableShader::init()
{
    _cgFP = LoadCgSourceProgram(g_context, "one_plane.cg", CG_PROFILE_FP30, 
                                "main"); 
    _dataParam = cgGetNamedParameter(_cgFP, "data");
    _tfParam = cgGetNamedParameter(_cgFP, "tf");
    _renderParam = cgGetNamedParameter(_cgFP, "render_param");
}

void NvColorTableShader::bind(Texture2D *plane, TransferFunction *tf)
{
    cgGLSetTextureParameter(_dataParam, plane->id());
    cgGLSetTextureParameter(_tfParam, tf->id());
    cgGLEnableTextureParameter(_dataParam);
    cgGLEnableTextureParameter(_tfParam);
    cgGLSetParameter4f(_renderParam, 0., 0., 0., 0.);

    cgGLBindProgram(_cgFP);
    cgGLEnableProfile(CG_PROFILE_FP30);
}

void NvColorTableShader::unbind()
{
    cgGLDisableProfile(CG_PROFILE_FP30);
    cgGLDisableTextureParameter(_dataParam);
    cgGLDisableTextureParameter(_tfParam);
}
